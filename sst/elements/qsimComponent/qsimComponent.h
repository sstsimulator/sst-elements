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

#ifndef QSIM_COMPONENT_H
#define QSIM_COMPONENT_H

#include <sst/core/sst_types.h>
//#include <sst/core/serialization/element.h>
#include <assert.h>

#include <sst/core/component.h>
#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/timeConverter.h>

#include <sst/core/interfaces/simpleMem.h>


#include <qsim.h>

#include <string>

namespace SST {
class Params;
namespace QsimComponent {
  class qsimComponent : public Component {
  public:
    qsimComponent(ComponentId_t id, Params &p);
    qsimComponent();

    void setup();
    void init(unsigned phase);
    void finish();

    friend class boost::serialization::access;
    template <class Archive>
      void save(Archive &ar, const unsigned version) const
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
      ar & BOOST_SERIALIZATION_NVP(stateFile);
      ar & BOOST_SERIALIZATION_NVP(appFile);
      ar & BOOST_SERIALIZATION_NVP(clockFreq);
      ar & BOOST_SERIALIZATION_NVP(hwThreadId);
      ar & BOOST_SERIALIZATION_NVP(icount);
      ar & BOOST_SERIALIZATION_NVP(rcount);
      ar & BOOST_SERIALIZATION_NVP(memLink);
      ar & BOOST_SERIALIZATION_NVP(iMemLink);
      ar & BOOST_SERIALIZATION_NVP(stalled);
      ar & BOOST_SERIALIZATION_NVP(lock);
      ar & BOOST_SERIALIZATION_NVP(pc);
    }

    template <class Archive>
      void load(Archive &ar, const unsigned version)
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
      ar & BOOST_SERIALIZATION_NVP(stateFile);
      ar & BOOST_SERIALIZATION_NVP(appFile);
      ar & BOOST_SERIALIZATION_NVP(clockFreq);
      ar & BOOST_SERIALIZATION_NVP(hwThreadId);
      ar & BOOST_SERIALIZATION_NVP(icount);
      ar & BOOST_SERIALIZATION_NVP(rcount);
      ar & BOOST_SERIALIZATION_NVP(memLink);
      ar & BOOST_SERIALIZATION_NVP(iMemLink);
      ar & BOOST_SERIALIZATION_NVP(stalled);
      ar & BOOST_SERIALIZATION_NVP(lock);
      ar & BOOST_SERIALIZATION_NVP(pc);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

  private:
    void handleEvent(Interfaces::SimpleMem::Request *);
    void handleEvent(Event *);
    bool clockTick(Cycle_t);
    bool timerTick(Cycle_t);

    Qsim::OSDomain *osd;
    Interfaces::SimpleMem *memLink, *iMemLink;
    Link *ipiRingIn, *ipiRingOut;

    unsigned hwThreadId;
    unsigned long icount, rcount, mcount;
    uint64_t pc;

    std::string stateFile, appFile, clockFreq;

    bool stalled, translating;
    int lock;

    // Callbacks for qsim
    int atomic_cb(int c);
    int mem_cb(int c, uint64_t v, uint64_t p, uint8_t s, int w);
    void trans_cb(int c);
    void inst_cb(int c, uint64_t v, uint64_t p, uint8_t l, const uint8_t *b,
                 enum inst_type t);
    int int_cb(int c, uint8_t v);
    int magic_cb(int c, uint64_t rax);

    Output out;
};

}}

#endif
