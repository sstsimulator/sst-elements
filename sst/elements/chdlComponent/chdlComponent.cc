// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "chdlComponent.h"

#include <chdl/egress.h>
#include <chdl/ingress.h>

#include <sst/core/debug.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "ldnetl.h"
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

chdlComponent::chdlComponent(ComponentId_t id, Params &p):
  Component(id), tog(0)
{
  debugLevel = p.find_integer("debugLevel", 1);
  int debug(p.find_integer("debug", 0));
  out.init("", debugLevel, 0, Output::output_location_t(debug));

  bool found;
  netlFile = p.find_string("netlist", "", found);
  if (!found) _abort(chdlComponent, "No netlist specified.\n");

  clockFreq = p.find_string("clockFreq", "2GHz");
  memFile = p.find_string("memInit", "");
  core_id = p.find_integer("id", 0);
  core_count = p.find_integer("cores", 1);

  string vcdFilename = p.find_string("vcd", "", dumpVcd);
  if (dumpVcd) vcd.open(vcdFilename);

  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

  memLink = dynamic_cast<SimpleMem*>(
    loadModuleWithComponent("memHierarchy.memInterface", this, p)
  );

  typedef SimpleMem::Handler<chdlComponent> mh;
  if (!memLink->initialize("memLink",new mh(this, &chdlComponent::handleEvent)))
      _abort(chdlComponent, "Unable to initialize Link memLink\n");

  typedef Clock::Handler<chdlComponent> ch;
  registerClock(clockFreq, new ch(this, &chdlComponent::clockTick));
}

// Used by serialization.
chdlComponent::chdlComponent(): Component(-1) {}

void chdlComponent::setup() {}

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

void chdlComponent::init_io_pre(const string &port) {
  char s[80];
  vector<char*> t;
  strncpy(s, port.c_str(), 80);
  tok(t, s, "_");
  if (!strncmp(t[0], "simplemem", 80)) {
    if (t.size() != 4)
      _abort(chdlComponent, "Malformed IO port name in netlist: %s\n",
             port.c_str());

    unsigned id;
    if (ports.find(t[3]) != ports.end()) id = ports[t[3]];
    else { id = ports.size(); ports[t[3]] = id; }
  }
}

void chdlComponent::init_io(const string &port, vector<chdl::node> &v) {
  char s[80];
  vector<char*> t;
  strncpy(s, port.c_str(), 80);
  tok(t, s, "_");

  if (!strncmp(t[0], "simplemem", 80)) {
    unsigned id(ports[t[3]]);
    out.debug(_L0_, "Found a simplemem io: %s\n", port.c_str());
    if (!strncmp(t[2], "req", 80)) {
      if (!strncmp(t[1], "ready", 80)) v[0] = Lit(1);
      else if (!strncmp(t[1], "valid", 80)) Egress(req[id].valid, v[0]);
      else if (!strncmp(t[1], "addr", 80)) EgressInt(req[id].addr, v);
      else if (!strncmp(t[1], "wr", 80)) Egress(req[id].wr, v[0]);
      else if (!strncmp(t[1], "data", 80)) EgressInt(req[id].data, v);
      else if (!strncmp(t[1], "size", 80)) EgressInt(req[id].size, v);
      else if (!strncmp(t[1], "id", 80)) EgressInt(req[id].id, v);
      else if (!strncmp(t[1], "llsc", 80)) Egress(req[id].llsc, v[0]);
      else if (!strncmp(t[1], "locked", 80)) Egress(req[id].locked, v[0]);
      else if (!strncmp(t[1], "uncached", 80)) Egress(req[id].uncached, v[0]);
      else _abort(chdlComponent, "Invalid simplemem req port: %s\n", t[1]);
    } else if (!strncmp(t[2], "resp", 80)) {
      if (id + 1 > resp.size())
        out.output("ERROR: id+1 exceeds resp.size().\n");

      if (!strncmp(t[1], "ready", 80)) Egress(resp[id].ready, v[0]);
      else if (!strncmp(t[1], "valid", 80)) v[0] = Ingress(resp[id].valid);
      else if (!strncmp(t[1], "wr", 80)) v[0] = Ingress(resp[id].wr);
      else if (!strncmp(t[1], "data", 80)) IngressInt(v, resp[id].data);
      else if (!strncmp(t[1], "id", 80)) IngressInt(v, resp[id].id);
      else if (!strncmp(t[1], "llsc", 80)) IngressInt(v, resp[id].llsc);
      else if (!strncmp(t[1], "llscsuc", 80)) IngressInt(v, resp[id].llsc_suc);
      else _abort(chdlComponent, "Invalid simplemem resp port: %s\n", t[1]);
    } else {
      _abort(chdlComponent, "Malformed IO port name in netlist: %s\n",
             port.c_str());
    }
  } else if (!strncmp(t[0], "counter", 80)) {
    counters[port] = new unsigned long();
    EgressInt(*counters[port], v);
  } else if (!strncmp(t[0], "id", 80) && t.size() == 1) {
    for (unsigned i = 0, mask = 1; i < v.size(); ++i, mask <<= 1)
      v[i] = Lit((core_id & mask) != 0);
  } else if (!strncmp(t[0], "cores", 80) && t.size() == 1) {
    for (unsigned i = 0, mask = 1; i < v.size(); ++i, mask <<= 1)
      v[i] = Lit((core_count & mask) != 0);    
  }
}

void chdlComponent::init(unsigned phase) {
  if (phase == 0) {
    map<string, vector<node> > outputs;
    map<string, vector<node> > inputs;
    map<string, vector<tristatenode> > inout;

    cd = push_clock_domain();
    ldnetl(outputs, inputs, inout, netlFile, dumpVcd);

    out.debug(_L0_, "chdlComponent init: loaded design \"%s\"\n",
              netlFile.c_str());

    // Set up egress/ingress nodes for req and resp
    for (auto x : inputs) init_io_pre(x.first);
    for (auto x : outputs) init_io_pre(x.first);

    for (auto &p : ports)
      out.debug(_L0_, "chdlComponent init: port \"%s\" id=%u\n",
                 p.first.c_str(), p.second);

    req.resize(ports.size());
    resp.resize(ports.size());
    responses_this_cycle.resize(ports.size());

    for (auto x : inputs) init_io(x.first, x.second);
    for (auto x : outputs) init_io(x.first, x.second);

    pop_clock_domain();

    out.debug(_L0_, "chdlComponent init: initialized clock domain %u\n", cd);
    out.debug(_L0_, "chdlComponent init: finished optimizing.\n");

  } else if (phase == 1) {
    if (dumpVcd) {
      print_vcd_header(vcd);
      print_time(vcd);
    }
    if (cd == 1) {
      optimize();
      for (unsigned i = 0; i < tickables().size(); ++i) {
        out.output("cdomain %u: %lu\n", i, tickables()[i].size());
      }
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
  // TODO: support queuing of responses
  unsigned port = portMap[req->id];

  // This is a temporary hack. We should be queueing up responses when they
  // arrive more than once per clock cycle, but for now we'll just tick the
  // clock and pretend time has passed.
  if (responses_this_cycle[port]) { clockTick(0); clockTick(0); }

  if (/*sp[0].ready*/1) resp[port].valid = true;
  else _abort(chdlComponent, "response arrived when receiver not ready");

  resp[port].wr = req->cmd == SimpleMem::Request::WriteResp;
  if (!resp[port].wr) {
    resp[port].data = 0;
    for (unsigned i = 0; i < req->data.size(); ++i)
      resp[port].data |= req->data[i] << 8*i;
  }
  resp[port].id = idMap[req->id];

  resp[port].llsc = ((req->flags & SimpleMem::Request::F_LLSC) != 0);
  resp[port].llsc_suc = ((req->flags & SimpleMem::Request::F_LLSC_RESP) != 0);

  out.debug(_L1_, "Response arrived on port %d for req %d, wr = %d, "
                  "data = %u, size = %lu, datasize = %lu, flags = %x\n",
               int(port), int(req->id), resp[port].wr,
               (unsigned)resp[port].data, req->size, req->data.size(),
               req->flags);

  delete req;

  ++responses_this_cycle[port];
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
  if (tog) {
    tog = !tog;
    for (auto &t : tickables()[cd]) t->tick(cd);
    for (unsigned i = 0; i < resp.size(); ++i) resp[i].valid = 0;
    for (auto &t : tickables()[cd]) t->tock(cd);
    for (auto &t : tickables()[cd]) t->post_tock(cd);
    ++now[cd];
    if (cd == 1) ++now[0];
  } else {
    tog = !tog;
    for (unsigned i = 0; i < req.size(); ++i) {
      if (responses_this_cycle[i] > 1)
        out.output("ERROR: %u responses on port %u this cycle.\n",
                   responses_this_cycle[i], i);
      responses_this_cycle[i] = 0;
    }

    if (dumpVcd) print_taps(vcd, cd);

    for (auto &t : tickables()[cd]) t->pre_tick(cd);

    // Handle requests
    for (unsigned i = 0; i < req.size(); ++i) {
      if (req[i].valid) {

        int flags = (req[i].uncached ? SimpleMem::Request::F_NONCACHEABLE : 0) |
                    (req[i].locked ? SimpleMem::Request::F_LOCKED : 0) |
                    (req[i].llsc ? SimpleMem::Request::F_LLSC : 0);

        SimpleMem::Request *r;
        if (req[i].wr) {
          vector<uint8_t> dVec;
          for (unsigned j = 0; j < req[i].size; ++j) {
            dVec.push_back((req[i].data >> 8*j) & 0xff);
          }
          r = new SimpleMem::Request(
            SimpleMem::Request::Write, req[i].addr, req[i].size, dVec, flags
          );
        } else {
          r = new SimpleMem::Request(
            SimpleMem::Request::Read, req[i].addr, req[i].size, flags
          );
        }

        out.debug(_L0_, "Req %d on port %u to %08lx: ",
                  (int)r->id, i, req[i].addr);

        if (req[i].wr)
          out.debug(_L0_, "Write %lu, ", req[i].data); 
        else
          out.debug(_L0_, "Read, ");

        out.debug(_L0_, "flags=%x\n", flags);

        if (req[i].wr && req[i].addr == 0x80000008) consoleOutput(req[i].data);

        out.debug(_L3_, "SimpleMem %d = CHDL ID %d\n",
                  (int)r->id, (int)req[i].id);
        idMap[r->id] = req[i].id;

        portMap[r->id] = i;

        memLink->sendRequest(r);
      }
    }

    if (dumpVcd) print_time(vcd);
  }

  return false;
}

BOOST_CLASS_EXPORT(chdlComponent);

static Component* create_chdlComponent(ComponentId_t id, Params &p) {
  return new chdlComponent(id, p);
}

static const ElementInfoParam component_params[] = {
  {"clockFreq", "Clock rate", "2GHz"},
  {"netlist", "Filename of CHDL .nand netlist", ""},
  {"debug", "Destination for debugging output", "0"},
  {"debugLevel", "Level of verbosity of output", "1"},
  {"memInit", "File containing initial memory contents", ""},
  {"id", "Device ID passed to \"id\" input, if present", "0"},
  {"cores", "Number of devices IDs passed to \"cores\" input, if present", "0"},
  {"vcd", "Filename of .vcd waveform file for this component's taps", ""},
  {NULL, NULL, NULL}
};

static const ElementInfoPort component_ports[] = {
    {"memLink", "Main Memory Link", NULL},
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
