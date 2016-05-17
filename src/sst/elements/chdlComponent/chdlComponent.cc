// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// #define CHDL_FASTSIM // Support for new fastsim branch of CHDL
// #define CHDL_TRANS   // (buggy) support for translation in fastsim

#include "sst_config.h"
#include "chdlComponent.h"

#include <chdl/egress.h>
#include <chdl/ingress.h>

#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include <cstring>

#include <iostream>

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::ChdlComponent;
using namespace chdl;
using namespace std;

// Debugging level macros similar to ones used in memHierarchy
#define _L0_ CALL_INFO,0,0
#define _L1_ CALL_INFO,1,0
#define _L2_ CALL_INFO,2,0
#define _L3_ CALL_INFO,3,0
#define _L4_ CALL_INFO,4,0
#define _L5_ CALL_INFO,5,0
#define _L6_ CALL_INFO,6,0
#define _L7_ CALL_INFO,7,0
#define _L8_ CALL_INFO,8,0
#define _L9_ CALL_INFO,9,0
#define _L10_ CALL_INFO,10,0


// A re-implementation of the CHDL EgressInt and IngressInt functions using C++
// vectors instead of vecs, trading run time configurability for compile time
// error detection.
template <typename T> void EgressInt(T &x, vector<node> v)
{
  x = 0;

  for (unsigned i = 0; i < v.size(); ++i)
    EgressFunc(
      [i, &x](bool val){
        if (val) x |= (1ull<<i); else x &= ~(1ull<<i);
      },
      v[i]
  );
}

template <typename T> void IngressInt(vector<node> &v, T &x) {
  for (unsigned i = 0; i < v.size(); ++i)
    v[i] = IngressFunc(GetBit(i, x));
}

// This is embarrassing, but I'd rather do this than strtok_r.
static void tok(vector<char*> &out, char* in, const char* sep) {
  char *s(in);
  out.clear();
  for (unsigned i = 0; in[i]; ++i) {
    for (unsigned j = 0; sep[j]; ++j) {
      if (in[i] == sep[j]) {
        in[i] = 0;
        out.push_back(s);
        s = &in[i+1];
      }
    }
  }
  out.push_back(s);
}

template <typename T> void VecLit(vector<node> &v, T &x) {
  for (unsigned i = 0; i < v.size(); ++i)
    v[i] = Lit((x >> i)&1ull);
}

chdlComponent::chdlComponent(ComponentId_t id, Params &p):
  Component(id), tog(0), stopSim(0), registered(false)
{
  debugLevel = p.find_integer("debugLevel", 1);
  int debug(p.find_integer("debug", 0));
  out.init("", debugLevel, 0, Output::output_location_t(debug));

  bool found;
  netlFile = p.find_string("netlist", "", found);
  if (!found) Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "No netlist specified.\n");

  clockFreq = p.find_string("clockFreq", "2GHz");
  memFile = p.find_string("memInit", "");
  core_id = p.find_integer("id", 0);
  core_count = p.find_integer("cores", 1);

  string vcdFilename = p.find_string("vcd", "", dumpVcd);
  if (dumpVcd) vcd.open(vcdFilename);

  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
  registered = true;

  memLink = dynamic_cast<SimpleMem*>(
    loadModuleWithComponent("memHierarchy.memInterface", this, p)
  );

  typedef SimpleMem::Handler<chdlComponent> mh;
  if (!memLink->initialize("memLink",new mh(this, &chdlComponent::handleEvent)))
    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Unable to initialize Link memLink\n");

  typedef Clock::Handler<chdlComponent> ch;
  registerClock(clockFreq, new ch(this, &chdlComponent::clockTick));

  string memPorts = p.find_string("memPorts", "mem", found);
  vector<char*> port_names;
  char s[80];
  strncpy(s, memPorts.c_str(), 80);
  tok(port_names, s, ",");
  unsigned next_id = 0;
  for (auto x : port_names)
    ports[x] = next_id++;

  // Resize request and response vectors to accomodate all port IDs.
  req.resize(next_id);
  req_copy.resize(next_id);
  resp.resize(next_id);
  resp_q.resize(next_id);
  responses_this_cycle.resize(next_id);
  byte_sz.resize(next_id);
  data_sz.resize(next_id);
  // resp_eaten.resize(next_id);
}

// Used by serialization.
chdlComponent::chdlComponent(): Component(-1) {}

void chdlComponent::setup() {}

void chdlComponent::init(unsigned phase) {
  if (phase == 0) {
    cd = push_clock_domain();
    iface_t iface(Load(netlFile));

    if (dumpVcd) iface.tap();

    // Connect the memory ports.
    for (auto &p : ports) {
      unsigned idx(p.second);
      const string &s(p.first);
      unsigned n(iface.out[s + "_req_contents_mask"].size()),
               b(iface.out[s + "_req_contents_data_0"].size());

      byte_sz[idx] = b;
      data_sz[idx] = n;
      if (n > 64) cout << "ERROR: max bytes per word (64) exceded.\n";
      if (b % 8) cout << "ERROR: byte size must be multiple of 8.\n";

      // The request side
      req[idx].ready = true;
      iface.in[s + "_req_ready"][0] = Ingress(req[idx].ready);
      Egress(req[idx].valid, iface.out[s + "_req_valid"][0]);
      Egress(req[idx].wr, iface.out[s + "_req_contents_wr"][0]);
      Egress(req[idx].llsc, iface.out[s + "_req_contents_llsc"][0]);
      EgressInt(req[idx].addr, iface.out[s + "_req_contents_addr"]);
      EgressInt(req[idx].mask, iface.out[s + "_req_contents_mask"]);
      EgressInt(req[idx].id, iface.out[s + "_req_contents_id"]);
      req[idx].data.resize(n);
      for (unsigned i = 0; i < n; ++i) {
        ostringstream oss; oss << s << "_req_contents_data_" << i;
	EgressInt(req[idx].data[i], iface.out[oss.str()]);
      }

      // The response side
      Egress(resp[idx].ready, iface.out[s + "_resp_ready"][0]);
      iface.in[s + "_resp_valid"][0] = Ingress(resp[idx].valid);
      iface.in[s + "_resp_contents_wr"][0] = Ingress(resp[idx].wr);
      iface.in[s + "_resp_contents_llsc"][0] = Ingress(resp[idx].llsc);
      iface.in[s + "_resp_contents_llsc_suc"][0] = Ingress(resp[idx].llsc_suc);
      IngressInt(iface.in[s + "_resp_contents_id"], resp[idx].id);
      resp[idx].data.resize(n);
      for (unsigned i = 0; i < n; ++i) {
	ostringstream oss; oss << s << "_resp_contents_data_" << i;
	IngressInt(iface.in[oss.str()], resp[idx].data[i]);
      }
    }

    // Find auxiliary I/O
    for (auto &p : iface.out) {
      char s[80];
      vector<char*> t;
      strncpy(s, p.first.c_str(), 80);
      tok(t, s, "_");

      // Counters
      if (!strncmp(t[0], "counter", 80)) {
        counters[p.first] = new unsigned long;
	EgressInt(*counters[p.first], p.second);
      } else if (
        t.size() == 2 &&
        !strncmp(t[0], "stop", 80) &&
	!strncmp(t[1], "sim", 80)
      ) {
	// TODO: Implement stop_sim
      }
    }

    for (auto &p : iface.in) {
      // TODO: disable this? This displays inputs as taps in the VCD too.
      for (unsigned i = 0; i < p.second.size(); ++i)
        tap(p.first, p.second[i]);
      
      if (p.first == "id") {
	VecLit(p.second, core_count);
      } else if (p.first == "cores") {
	VecLit(p.second, core_id);
      }
    }

    pop_clock_domain();
  } else if (phase == 1) {
    if (dumpVcd) {
      print_vcd_header(vcd);
      print_time(vcd);
    }
    if (cd == 1) {
      optimize();
      for (unsigned i = 0; i < tickables().size(); ++i) {
        out.debug(_L1_, "cdomain %u: %lu regs.\n", i, tickables()[i].size());
      }
      #ifdef CHDL_TRANS
      init_trans();
      #endif
    }
  } else if (phase == 2 && memFile != "") {
    ifstream m(memFile);

    unsigned limit(16*1024), addr(0);
    while (!!m && --limit) {
      char buf[1024];
      m.read(buf, 1024);
      SimpleMem::Request *req =
        new SimpleMem::Request(SimpleMem::Request::Write, addr, 1024);
      req->setPayload((unsigned char *)buf, 1024);
      memLink->sendInitData(req);
      addr += 1024;
    }
  }
}

void chdlComponent::finish() {
  unsigned long simCycle(Simulation::getSimulation()->getCurrentSimCycle());  

  for (auto &x : counters)
    out.output("CHDL %u counter \"%s\": %lu\n",
               core_id, x.first.c_str(), *x.second);

  out.debug(_L2_, "%lu sim cycles\n", simCycle);
}


void chdlComponent::handleEvent(Interfaces::SimpleMem::Request *req) {
  unsigned port = portMap[req->id];

  // TODO: valid is always cleared; we should check that the requester was
  // actually ready.
  if (responses_this_cycle[port] || resp[port].valid) {
    out.debug(_L2_, "Adding an entry to resp_q[%u]. ", port);
    resp_q[port].push(req);
    out.debug(_L2_, "I now has %u entries.\n", (unsigned)resp_q[port].size());
  } else {
    resp[port].wr = req->cmd == SimpleMem::Request::WriteResp;

    resp[port].valid = true;

    if (!resp[port].wr) {
      if (req->addr & 0x80000000) mmio_rd(req->data, req->addr);

      unsigned idx = 0;
      for (unsigned i = 0; i < data_sz[port]; ++i) {
	resp[port].data[i] = 0;
        for (unsigned j = 0; j < byte_sz[port]; j += 8) {
	  resp[port].data[i] <<= 8;
	  resp[port].data[i] |= req->data[idx];
	  idx++;
	}
      }
    }
    resp[port].id = idMap[req->id];

    resp[port].llsc = ((req->flags & SimpleMem::Request::F_LLSC) != 0);
    resp[port].llsc_suc = ((req->flags & SimpleMem::Request::F_LLSC_RESP) != 0);

    out.debug(_L1_, "Response arrived on port %d for req %d, wr = %d, "
                    "size = %lu, datasize = %lu, flags = %x\n",
                 int(port), int(req->id), resp[port].wr,
                 req->size, req->data.size(), req->flags);

    delete req;

    ++responses_this_cycle[port];
  }
}

void chdlComponent::consoleOutput(char c) {
 if (c == '\n') {
   out.output("%lu %u OUTPUT> %s\n", (unsigned long)now[cd], core_id,
              outputBuffer.c_str());
   outputBuffer.clear();
 } else {
   outputBuffer = outputBuffer + c;
 }
}

bool chdlComponent::clockTick(Cycle_t c) {
  #ifdef CHDL_FASTSIM
  #ifdef CHDL_TRANS
  #define tick_arg trans_evaluator()
  #else
  evaluator_t tick_arg(default_evaluator(cd));
  #endif
  #else
  cdomain_handle_t tick_arg(cd);
  #endif

  if (tog) {
    if (dumpVcd) print_taps(vcd, tick_arg);

    #ifdef CHDL_TRANS
    recompute_logic_trans(cd);
    tick_trans(cd);
    tock_trans(cd);
    post_tock_trans(cd);
    #else
    for (auto &t : tickables()[cd]) t->tick(tick_arg);
    for (auto &t : tickables()[cd]) t->tock(tick_arg);
    for (auto &t : tickables()[cd]) t->post_tock(tick_arg);
    #endif

    if (dumpVcd) print_time(vcd);

    for (unsigned i = 0; i < resp.size(); ++i) resp[i].valid = 0;
    
    ++now[cd];
    if (cd == 1) ++now[0];
  } else {
    // for (unsigned i = 0; i < resp.size(); ++i) resp[i].valid = 0;
    
    for (unsigned i = 0; i < req.size(); ++i) {
      if (responses_this_cycle[i] == 0 && !resp_q[i].empty()) {
        // Handle enqueued event.
        handleEvent(resp_q[i].front());
        resp_q[i].pop();
      } else if (responses_this_cycle[i] > 1) {
        out.output("ERROR: %u responses on port %u this cycle.\n",
                   responses_this_cycle[i], i);
      }
      responses_this_cycle[i] = 0;
    }

    #ifdef TRANS
    pre_tick(cd);
    #else
    for (auto &t : tickables()[cd]) t->pre_tick(tick_arg);
    #endif

    // Copy new requests
    for (unsigned i = 0; i < req.size(); ++i) {
      if (req[i].ready && req[i].valid) {
	req_copy[i] = req[i];
      }
    }
    
    // Handle requests
    for (unsigned i = 0; i < req.size(); ++i) {
      if (req_copy[i].valid && (!req_copy[i].wr || (req_copy[i].mask != 0))) {
        int flags = (req_copy[i].llsc ? SimpleMem::Request::F_LLSC : 0);
	
        uint64_t mask(req_copy[i].mask),
	         mask_lsb_only(mask&-mask), mask_lsb_pos(LOG2(mask_lsb_only)),
                 mask_msb_only((mask+mask_lsb_only) & -(mask+mask_lsb_only)),
                 mask_msb_pos(LOG2(mask_msb_only)),
                 mask_cluster((mask_lsb_only-1) ^ (mask_msb_only-1)),
                 mask_cluster_len(mask_msb_pos - mask_lsb_pos);

	req_copy[i].mask ^= mask_cluster;
	
	bool wr(req_copy[i].wr);
        unsigned b(byte_sz[i]), n(data_sz[i]),
	         req_size(wr ? b * mask_cluster_len / 8 : n * b / 8);
	unsigned long addr(req_copy[i].addr * n * (b / 8));

        // Generate SimpleMem Request
	SimpleMem::Request *r;
	if (wr) {
	  // Write addresses are offset by the mask LSB.
	  addr += mask_lsb_pos * b / 8;
          vector<uint8_t> dVec(n * b / 8);

	  unsigned idx = 0;
	  for (unsigned j = 0; j < mask_cluster_len; ++j) {
	    for (unsigned k = 0; k < b; k += 8) {
	      dVec[idx] = (req_copy[i].data[j] >> k)&0xff;
	      ++idx;
	    }
	  }

          if (addr & 0x80000000) mmio_wr(dVec, addr);
	  
	  r = new SimpleMem::Request(
	    SimpleMem::Request::Write, addr, req_size, dVec, flags
	  );
	} else {
	  r = new SimpleMem::Request(
            SimpleMem::Request::Read, addr, req_size, flags
	  );
	}

	idMap[r->id] = req_copy[i].id;
	portMap[r->id] = i;

	memLink->sendRequest(r);

	// Set valid bit based on whether req has been fully handled.
	req_copy[i].valid = wr && (req_copy[i].mask != 0);
	req[i].ready = !req_copy[i].valid;
      }
    }

    // If the CPU is exiting the simulation, unregister.
    if (stopSim && registered) {
      out.output("Core %u UNREGISTERING EXIT\n", core_id);
      registered = false;
      unregisterExit();
    }
  }

  tog = !tog;
  
  return false;
}

uint32_t chdlComponent::mmio_rd(uint64_t addr) {
  uint32_t data(0);

  switch (addr) {
    case 0x88000004: data = core_id; break;
    case 0x88000008: data = core_count; break;
  }
  
  out.debug(_L2_, "MMIO RD addr = 0x%08lx, data = 0x%08x", addr, data);
  
  return data;
}

void chdlComponent::mmio_wr(uint32_t data, uint64_t addr) {
  out.debug(_L2_, "MMIO WR addr = 0x%08lx, data = 0x%08x", addr, data);

  switch (addr) {
    case 0x80000004: stopSim = true; break;
    case 0x80000008: consoleOutput(data); break;
  }
}

void chdlComponent::mmio_rd(vector<uint8_t> &d, uint64_t addr) {
  // Assume addr aligned on 4-byte boundary
  for (unsigned i = 0; i < d.size(); i += 4) {
    uint32_t x(mmio_rd(addr + i));

    for (unsigned j = 0; j < 4; ++j)
      d[i + j] = (x >> (8 * j)) & 0xff;
  }
}

void chdlComponent::mmio_wr(const vector<uint8_t> &d, uint64_t addr) {
  for (unsigned i = 0; i < d.size(); i += 4) {
    uint32_t x(d[i] | (d[i + 1] << 8) | (d[i + 2] << 16) | d[i + 3] << 24);
    mmio_wr(x, addr + i);
  }
}

static Component* create_chdlComponent(ComponentId_t id, Params &p) {
  return new chdlComponent(id, p);
}

static const ElementInfoParam component_params[] = {
  {"clockFreq", "Clock rate", "2GHz"},
  {"netlist", "Filename of CHDL .nand netlist", ""},
  {"debug", "Destination for debugging output", "0"},
  {"debugLevel", "Level of verbosity of output", "1"},
  {"memPorts", "Memory port names, comma-separated.", "mem"},
  {"memInit", "File containing initial memory contents", ""},
  {"id", "Device ID passed to \"id\" input, if present", "0"},
  {"cores", "Max device ID plus 1.", "0"},
  {"vcd", "Filename of .vcd waveform file for this component's taps", ""},
  {NULL, NULL, NULL}
};

static const ElementInfoPort component_ports[] = {
    {"memLink", "Main memory link", NULL},
    {NULL, NULL, NULL}
};

static const ElementInfoComponent components[] = {
  { "chdlComponent",
    "Register transfer level modeling of devices.",
    NULL,
    create_chdlComponent,
    component_params,
    component_ports,
    COMPONENT_CATEGORY_PROCESSOR
  },
  {NULL, NULL, NULL, NULL, NULL, NULL, 0}
};

extern "C" {
  ElementLibraryInfo chdlComponent_eli = {
    "chdlComponent",
    "CHDL netlist execution component.",
    components,
  };
}
