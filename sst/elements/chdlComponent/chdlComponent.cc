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

// A re-implementation of the CHDL function, using C++ vectors instead of vecs.
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
  Component(id)
{
  out.init("", 0, 0, Output::STDOUT);

  bool found;
  netlFile = p.find_string("netlist", "", found);
  if (!found) _abort(chdlComponent, "No netlist specified.\n");

  clockFreq = p.find_string("clockFreq", "2GHz", found);

  memLink = dynamic_cast<SimpleMem*>(
    loadModuleWithComponent("memHierarchy.memInterface", this, p)
  );

  typedef SimpleMem::Handler<chdlComponent> mh;
  if (!memLink->initialize("memLink",new mh(this, &chdlComponent::handleEvent)))
      _abort(chdlComponent, "Unable to load Link memLink\n");

  typedef Clock::Handler<chdlComponent> ch;
  registerClock(clockFreq, new ch(this, &chdlComponent::clockTick));
}

// Used by serialization.
chdlComponent::chdlComponent(): Component(-1) {}

void chdlComponent::setup() {}

// This is embarrassing, but I'd rather do this than strtok_r.
static void tok(vector<char*> &out, char* in, const char* sep) {
  char *s(in);
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

void chdlComponent::init_io(const string &port, vector<chdl::node> &v) {
  char s[80];
  vector<char*> t;
  strncpy(s, port.c_str(), 80);
  tok(t, s, "_");
  if (!strncmp(t[0], "simplemem", 80)) {
    out.output("Found a simplemem io: %s\n", port.c_str());
    if (t.size() != 4)
      _abort(chdlComponent, "Malformed IO port name in netlist: %s\n",
             port.c_str());

    unsigned id(0); { istringstream iss(t[3]); iss >> id; }
    if (!strncmp(t[2], "req", 80)) {
      out.output("  req port\n");
      if (id + 1 > req.size()) req.resize(id + 1);

      if (!strncmp(t[1], "ready", 80)) v[0] = Lit(1);
      else if (!strncmp(t[1], "valid", 80)) Egress(req[id].valid, v[0]);
      else if (!strncmp(t[1], "addr", 80)) EgressInt(req[id].addr, v);
      else if (!strncmp(t[1], "wr", 80)) Egress(req[id].wr, v[0]);
      else if (!strncmp(t[1], "data", 80)) EgressInt(req[id].data, v);
      else if (!strncmp(t[1], "size", 80)) EgressInt(req[id].size, v);
      else if (!strncmp(t[1], "id", 80)) EgressInt(req[id].id, v);
      else if (!strncmp(t[1], "exclusive", 80)) Egress(req[id].exclusive, v[0]);
      else if (!strncmp(t[1], "locked", 80)) Egress(req[id].locked, v[0]);
      else if (!strncmp(t[1], "uncached", 80)) Egress(req[id].uncached, v[0]);
      else _abort(chdlComponent, "Invalid simplemem req port: %s\n", t[1]);
    } else if (!strncmp(t[2], "resp", 80)) {
      out.output("  resp port\n");
      if (id + 1 > resp.size()) resp.resize(id + 1);

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
  }
}

void chdlComponent::init(unsigned phase) {
  if (phase != 0) return;

  map<string, vector<node> > outputs;
  map<string, vector<node> > inputs;
  map<string, vector<tristatenode> > inout;

  ldnetl(outputs, inputs, inout, netlFile);

  out.output("chdlComponent init: loaded design \"%s\"\n", netlFile.c_str());

  // Set up egress/ingress nodes for req and res
  for (auto x : inputs) init_io(x.first, x.second);
  for (auto x : outputs) init_io(x.first, x.second);

  EgressInt(test_pc, outputs["pc"]);

  optimize();
 
  out.output("chdlComponent init: finished optimizing.\n");
}

void chdlComponent::finish() {
  unsigned long simCycle(Simulation::getSimulation()->getCurrentSimCycle());  

  // TODO: counters
  out.output("%lu sim cycles\n", simCycle);
}


void chdlComponent::handleEvent(Interfaces::SimpleMem::Request *req) {
  // TODO: support multiple responses
  if (resp[0].ready) resp[0].valid = true;
  else _abort(chdlComponent, "response arrived when receiver not ready");

  resp[0].wr = req->cmd == SimpleMem::Request::WriteResp;
  if (!resp[0].wr) {
    resp[0].data = 0;
    for (unsigned i = 0; i < req->data.size(); ++i)
      resp[0].data |= req->data[i] << 8*i;
  }
  resp[0].id = req->id;

  out.output("Response arrived for req %d, wr = %d, data = %lu, size = %lu, datasize = %lu\n", int(req->id), resp[0].wr, (unsigned long)resp[0].data, req->size, req->data.size());

  delete req;
}


bool chdlComponent::clockTick(Cycle_t c) {
  advance();

  resp[0].valid = 0;

  // Handle requests
  for (unsigned i = 0; i < req.size(); ++i) {
    if (req[i].valid) {
      out.output("Req to %08lx: ", req[i].addr);
      if (req[i].wr)
        out.output("Write %lu\n", req[i].data); 
      else
        out.output("Read\n");

      int flags = (req[i].uncached ? SimpleMem::Request::F_UNCACHED : 0) |
                  (req[i].locked ? SimpleMem::Request::F_LOCKED : 0) |
                  (req[i].exclusive ? SimpleMem::Request::F_EXCLUSIVE : 0);

      SimpleMem::Request *r;
      if (req[i].wr) {
        vector<uint8_t> dVec;
        for (unsigned j = 0; j < req[i].size; ++j) {
          dVec.push_back((req[i].data >> 8*j) & 0xff);
        }
        r = new SimpleMem::Request(
          SimpleMem::Request::Write, req[i].addr, req[i].size, dVec, flags
        );
        out.output("  write size = %d(%d)\n",
                   (int)req[i].size, (int)dVec.size());
      } else {
        r = new SimpleMem::Request(
          SimpleMem::Request::Read, req[i].addr, req[i].size, flags
        );
      }

      out.output("  Req ID: %d\n", int(r->id));

      memLink->sendRequest(r);
    }
  }

  out.output("pc: %u\n", test_pc);

  return false;
}

BOOST_CLASS_EXPORT(chdlComponent);

static Component* create_chdlComponent(ComponentId_t id, Params &p) {
  return new chdlComponent(id, p);
}

static const ElementInfoParam component_params[] = {
  {"clockFreq", "Clock rate", "1GHz"},
  {"netlist", "CHDL .nand netlist", ""},
  {NULL, NULL, NULL}
};

static const ElementInfoPort component_ports[] = {
    {"mem", "Main Memory Link", NULL},
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
