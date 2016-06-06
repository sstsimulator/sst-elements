// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "qsimComponent.h"

#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include <iostream>

#include <qsim.h>
#include <qsim-load.h>

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::QsimComponent;
using namespace Qsim;
using namespace std;

namespace SST { namespace QsimComponent {
class IPIEvent : public Event {
public:
  IPIEvent(int dest, uint8_t vec): dest(dest), vec(vec) {}
  int dest;
  uint8_t vec;

private:
  friend class boost::serialization::access;
  IPIEvent() {}
  template <class A> void serialize(A &ar, const unsigned int version) {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
    ar & BOOST_SERIALIZATION_NVP(dest);
    ar & BOOST_SERIALIZATION_NVP(vec);
  }
};
}}

qsimComponent::qsimComponent(ComponentId_t id, Params &p):
  Component(id), icount(0), rcount(0), mcount(0), stalled(false),
  translating(false), lock(0)
{
  out.init("", 0, 0, Output::STDOUT);

  bool found;
  stateFile = p.find_string("state", "", found);
  if (!found) out.fatal(CALL_INFO, -1, "State file not provided\n");

  appFile = p.find_string("app", "", found);
  if (!found) out.fatal(CALL_INFO, -1, "Application .tar file not provided\n");

  clockFreq = p.find_string("clock", "5GHz", found);

  hwThreadId = p.find_integer("hwthread", 0, found);
  if (!found) hwThreadId = 0;

  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

  memLink = dynamic_cast<SimpleMem*>(loadModuleWithComponent("memHierarchy.memInterface", this, p));
  assert(memLink);
  iMemLink = dynamic_cast<SimpleMem*>(loadModuleWithComponent("memHierarchy.memInterface", this, p));
  assert(iMemLink);

  typedef SimpleMem::Handler<qsimComponent> qc_mh;
  if ( !memLink->initialize("memLink",
          new qc_mh(this, &qsimComponent::handleEvent)) ) {
      out.fatal(CALL_INFO, -1, "Unable to load Link memLink\n");
  }
  if ( ! iMemLink->initialize("iMemLink", new qc_mh(this, &qsimComponent::handleEvent)) ) {
      delete iMemLink;
      iMemLink = NULL;
  }

  typedef Event::Handler<qsimComponent> qc_eh;
  ipiRingIn = configureLink("ipiRingIn",
                            new qc_eh(this, &qsimComponent::handleEvent));
  ipiRingOut = configureLink("ipiRingOut",
                             new qc_eh(this, &qsimComponent::handleEvent));
  assert(ipiRingIn && ipiRingOut);

  typedef Clock::Handler<qsimComponent> qc_ch;
  registerClock(clockFreq, new qc_ch(this, &qsimComponent::clockTick));
  registerClock("1kHz", new qc_ch(this, &qsimComponent::timerTick));
}

// Used by serialization.
qsimComponent::qsimComponent(): Component(-1) {}

void qsimComponent::setup() {}

void qsimComponent::init(unsigned phase) {
  if (phase != 0) return;

  osd = new OSDomain(stateFile.c_str());
  osd->connect_console(cout);
  out.output("qsimComponent %u init: loaded state\n", hwThreadId);
  load_file(*osd, appFile.c_str());
  out.output("qsimComponent %u init: loaded application\n", hwThreadId);

  osd->set_atomic_cb(this, &qsimComponent::atomic_cb);
  osd->set_int_cb(this, &qsimComponent::int_cb);
  osd->set_inst_cb(this, &qsimComponent::inst_cb);
  osd->set_mem_cb(this, &qsimComponent::mem_cb);
  osd->set_trans_cb(this, &qsimComponent::trans_cb);
  osd->set_magic_cb(this, &qsimComponent::magic_cb);

  out.output("qsimComponent init: callbacks set\n");

  if (hwThreadId == 0) {
    out.output("qsimComponent %u init: sending init data\n", hwThreadId);

    SimpleMem::Request *req = new SimpleMem::Request(SimpleMem::Request::Write, 0, 1024ul*1024*osd->get_ram_size_mb());
    req->setPayload(osd->get_ramdesc().mem_ptr, 1024ul*1024*osd->get_ram_size_mb());
    memLink->sendInitData(req);
    out.output("qsimComponent %u init: sent init data\n", hwThreadId);
  }
}

void qsimComponent::finish() {
  unsigned long simCycle(Simulation::getSimulation()->getCurrentSimCycle());  

  out.output("%lu instructions executed, ", icount);
  out.output("%lu outstanding reqs, %lu messages received,\n", rcount, mcount);
  out.output("%lu sim cycles\n", simCycle);
}


void qsimComponent::handleEvent(SimpleMem::Request *req) {
    if (--rcount == 0) stalled = false;
    ++mcount;

    for (unsigned i = 0; i < req->data.size(); ++i)
      osd->get_ramdesc().mem_ptr[req->addr + i] = req->data[i];

    delete req;
}


void qsimComponent::handleEvent(Event *event) {
  if (IPIEvent *e = dynamic_cast<IPIEvent*>(event)) {
    if (e->dest == -1) {
      out.output("qsimComponent %u handleEvent: End message.\n", hwThreadId);
      unregisterExit();
      ipiRingOut->send(e);
    } else if (unsigned(e->dest) == hwThreadId) {
      out.output("qsimComponent %u handleEvent: IPI arrived\n", hwThreadId);
      osd->interrupt(hwThreadId, e->vec);
      delete e;
    } else {
      ipiRingOut->send(e);
    }
  } else {
    out.fatal(CALL_INFO, -1, "Received unknown event type.\n");
  }
}

bool qsimComponent::clockTick(Cycle_t c) {
  if (!stalled) osd->run(hwThreadId, 1);

  return false;
}

bool qsimComponent::timerTick(Cycle_t c) {
  osd->timer_interrupt();

  return false;
}

int qsimComponent::atomic_cb(int c) {
  lock = 1;
  return 0;
}

int qsimComponent::mem_cb(int c, uint64_t v, uint64_t p, uint8_t s, int w) {
  if (p >= 0xf0000000) return 0; // ignore memory-mapped APIC access

  for (unsigned i = 0, increment; i < s; i += increment) {
    if      (s-i >= 8 && ((p+i) & 7) == 0) increment = 8;
    else if (s-i >= 4 && ((p+i) & 3) == 0) increment = 4;
    else if (s-i >= 2 && ((p+i) & 1) == 0) increment = 2;
    else increment = 1;

    SimpleMem::Request *req = new SimpleMem::Request(w ? SimpleMem::Request::Write : SimpleMem::Request::Read, p+i, increment);
    if (v && ((lock == 1 && !w) || (lock == 2 && w)) && i == 0) {
      req->flags |= (SimpleMem::Request::F_LOCKED);

      if (lock == 1 && !w) lock = 2;
      if (lock == 2 && w) lock = 0;
    }

    if (w) req->setPayload(osd->get_ramdesc().mem_ptr + p + i, increment);

    if (translating && !w && iMemLink) iMemLink->sendRequest(req);
    else memLink->sendRequest(req);

    stalled = !w;
    ++rcount;
  }

  return !w;
}

void qsimComponent::trans_cb(int c) {
  translating = true;
}

void qsimComponent::inst_cb(int c, uint64_t v, uint64_t p, uint8_t l,
                            const uint8_t *b, enum inst_type t)
{
  translating = false;
  pc = v;

#if 0
  for (unsigned i = 0, increment; i < l; i += increment) {
    if      (l-i >= 8 && ((p+i) & 7) == 0) increment = 8;
    else if (l-i >= 4 && ((p+i) & 3) == 0) increment = 4;
    else if (l-i >= 2 && ((p+i) & 1) == 0) increment = 2;
    else increment = 1;

    MemEvent *e = new MemEvent(this, p + i, Gets);
    e->setSize(increment);
    if (iMemLink) iMemLink->send(e);
    else memLink->send(e);
    stalled = true;
    ++rcount;
  }
#endif

  ++icount;
}

int qsimComponent::int_cb(int c, uint8_t v) {
  return 0;
}

int qsimComponent::magic_cb(int c, uint64_t a) {
  if (a == 0xfa11dead) {
    unregisterExit();
    ipiRingOut->send(new IPIEvent(-1, 0));
  }

  if ((a & 0xff000000) == 0x1d000000) {
    unsigned dest((a >> 8)&0xffff), vec((a & 0xff));
    ipiRingOut->send(new IPIEvent(dest, vec));
  }

  return 0;
}

BOOST_CLASS_EXPORT(SST::QsimComponent::IPIEvent);
BOOST_CLASS_EXPORT(SST::QsimComponent::qsimComponent);

static Component* create_qsimComponent(ComponentId_t id, Params &p) {
  return new qsimComponent(id, p);
}

static const ElementInfoParam component_params[] = {
  {"state",    "QSim saved state file.", NULL},
  {"app",      "Benchmark .tar file.", NULL},
  {"clock",    "Clock rate (default 2GHz).", "2GHz"},
  {"hwthread", "Hardware thread ID (default 0).", "0"},
  {NULL, NULL, NULL}
};

static const ElementInfoPort component_ports[] = {
    {"memLink", "Main Memory Link", NULL},
    {"iMemLink", "Alternate Memory Link for Translated Reads", NULL},
    {"ipiRingIn", "Inter-QSim Component Ring Network", NULL},
    {"ipiRingOut", "Inter-QSim Component Ring Network", NULL},
    {NULL, NULL, NULL}
};

static const ElementInfoComponent components[] = {
  { "qsimComponent",
    "Distributed QSim CPU model component.",
    NULL,
    create_qsimComponent,
    component_params,
    component_ports,
    COMPONENT_CATEGORY_PROCESSOR
  },
  {NULL, NULL, NULL, NULL, NULL, NULL, 0}
};

extern "C" {
  ElementLibraryInfo qsimComponent_eli = {
    "qsimComponent",
    "Distributed QSim CPU model component.",
    components,
  };
}
