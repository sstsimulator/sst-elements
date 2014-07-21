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
  Component(id), tog(0), vcd("chdl.vcd")
{
  out.init("", 0, 0, Output::STDOUT);

  debugLevel = p.find_integer("debugLevel", 1);

  bool found;
  netlFile = p.find_string("netlist", "", found);
  if (!found) _abort(chdlComponent, "No netlist specified.\n");

  clockFreq = p.find_string("clockFreq", "2GHz", found);
  memFile = p.find_string("memInit", "", found);

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
    if (debugLevel > 0)
      out.output("Found a simplemem io: %s\n", port.c_str());
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
      else _abort(chdlComponent, "Invalid simplemem resp port: %s\n", t[1]);
    } else {
      _abort(chdlComponent, "Malformed IO port name in netlist: %s\n",
             port.c_str());
    }
  } else if (!strncmp(t[0], "counter", 80)) {
    counters[port] = new unsigned long();
    EgressInt(*counters[port], v);
  }
}

void chdlComponent::init(unsigned phase) {
  if (phase == 0) {
    map<string, vector<node> > outputs;
    map<string, vector<node> > inputs;
    map<string, vector<tristatenode> > inout;

    cd = push_clock_domain();
    ldnetl(outputs, inputs, inout, netlFile);

    if (debugLevel > 0)
      out.output("chdlComponent init: loaded design \"%s\"\n",
                 netlFile.c_str());

    // Set up egress/ingress nodes for req and resp
    for (auto x : inputs) init_io_pre(x.first);
    for (auto x : outputs) init_io_pre(x.first);

    for (auto &p : ports)
      out.output("chdlComponent init: port \"%s\" id=%u\n",
                 p.first.c_str(), p.second);

    req.resize(ports.size());
    resp.resize(ports.size());
    responses_this_cycle.resize(ports.size());

    for (auto x : inputs) init_io(x.first, x.second);
    for (auto x : outputs) init_io(x.first, x.second);

    pop_clock_domain();

    optimize();

    out.output("chdlComponent init: initialized clock domain %u\n", cd);
 
    if (debugLevel > 0)
      out.output("chdlComponent init: finished optimizing.\n");
  } else if (phase == 1 && cd == 1) {
    print_vcd_header(vcd);
    print_time(vcd);
  } else if (phase == 2 && memFile != "") {
    ifstream m(memFile);

    unsigned limit(128*1024), addr(0);
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
    out.output("CHDL counter \"%s\": %lu\n", x.first.c_str(), *x.second);

  out.output("%lu sim cycles\n", simCycle);
}


void chdlComponent::handleEvent(Interfaces::SimpleMem::Request *req) {
  // TODO: support queuing responses
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

  if (debugLevel > 2) {
    out.output("Response arrived on port %d for req %d, wr = %d, data = %lu, "
               "size = %lu, datasize = %lu\n",
               int(port), int(req->id), resp[port].wr,
               (unsigned long)resp[port].data, req->size, req->data.size());
  }

  delete req;

  ++responses_this_cycle[port];
}


bool chdlComponent::clockTick(Cycle_t c) {
  if (tog) {
    tog = !tog;
    for (auto &t : tickables()[cd]) t->tick();
    for (auto &t : tickables()[cd]) t->tock();
    for (auto &t : tickables()[cd]) t->post_tock();
    if (cd == 1) ++now;
  } else {
    tog = !tog;
    for (unsigned i = 0; i < req.size(); ++i) {
      if (responses_this_cycle[i] > 1)
        out.output("ERROR: %u responses on port %u this cycle.\n",
                   responses_this_cycle[i], i);
      responses_this_cycle[i] = 0;
    }

    print_taps(vcd);

    for (auto &t : tickables()[cd]) t->pre_tick();

    for (unsigned i = 0; i < resp.size(); ++i) resp[i].valid = 0;

    // Handle requests
    for (unsigned i = 0; i < req.size(); ++i) {
      if (req[i].valid) {
        if (debugLevel > 0) {
          out.output("Req on port %u to %08lx: ", i, req[i].addr);
          if (req[i].wr)
            out.output("Write %lu\n", req[i].data); 
          else
            out.output("Read\n");
        }

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

        if (debugLevel > 3)
          out.output("SimpleMem %d = CHDL ID %d\n", (int)r->id, (int)req[i].id);
        idMap[r->id] = req[i].id;

        portMap[r->id] = i;

        memLink->sendRequest(r);
      }
    }

    // for (auto &t : tickables()[cd]) t->tick();
    // for (auto &t : tickables()[cd]) t->tock();
    // for (auto &t : tickables()[cd]) t->post_tock();
    // if (cd == 1) ++now;

    print_time(vcd);
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
  {"debugLevel", "Level of verbosity of output", "1"},
  {"memInit", "File containing initial memory contents.", ""},
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
