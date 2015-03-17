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

#ifndef CHDL_COMPONENT_H
#define CHDL_COMPONENT_H

#include <string>
#include <map>
#include <queue>

// TODO: perhaps CHDL shouldn't have things with names that conflict with C
// standard library macros. "assert" causes a problem here if the inclusion
// order isn't just so.
#undef assert
#include <chdl/chdl.h>
#undef assert

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

namespace SST {
class Params;

namespace ChdlComponent {
  struct reqdata {
    uint64_t addr, data, id, size;
    bool valid, wr, llsc, locked, uncached;

    friend class boost::serialization::access;
    template<class Archive>
      void serialize(Archive &ar, const unsigned version)
    {
      ar & BOOST_SERIALIZATION_NVP(addr);
      ar & BOOST_SERIALIZATION_NVP(data);
      ar & BOOST_SERIALIZATION_NVP(id);
      ar & BOOST_SERIALIZATION_NVP(size);
      ar & BOOST_SERIALIZATION_NVP(valid);
      ar & BOOST_SERIALIZATION_NVP(wr);
      ar & BOOST_SERIALIZATION_NVP(llsc);
      ar & BOOST_SERIALIZATION_NVP(locked);
      ar & BOOST_SERIALIZATION_NVP(uncached);
    }
  };

  struct respdata {
    uint64_t data, id;
    bool ready, valid, wr, llsc, llsc_suc;

    friend class boost::serialization::access;
    template<class Archive>
      void serialize(Archive &ar, const unsigned version)
    {
      ar & BOOST_SERIALIZATION_NVP(data);
      ar & BOOST_SERIALIZATION_NVP(id);
      ar & BOOST_SERIALIZATION_NVP(ready);
      ar & BOOST_SERIALIZATION_NVP(valid);
      ar & BOOST_SERIALIZATION_NVP(wr);
      ar & BOOST_SERIALIZATION_NVP(llsc);
      ar & BOOST_SERIALIZATION_NVP(llsc_suc);
    }
  };

  class chdlComponent : public Component {
  public:
    chdlComponent(ComponentId_t id, Params &p);
    chdlComponent();

    void setup();
    void init(unsigned phase);
    void finish();

    friend class boost::serialization::access;
    template <class Archive>
      void save(Archive &ar, const unsigned version) const
    {
      #if 0
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
      ar & BOOST_SERIALIZATION_NVP(memLink);
      ar & BOOST_SERIALIZATION_NVP(clockFreq);
      ar & BOOST_SERIALIZATION_NVP(netlFile);
      ar & BOOST_SERIALIZATION_NVP(memFile);
      ar & BOOST_SERIALIZATION_NVP(outputBuffer);
      ar & BOOST_SERIALIZATION_NVP(req);
      ar & BOOST_SERIALIZATION_NVP(resp); 
      ar & BOOST_SERIALIZATION_NVP(idMap);
      ar & BOOST_SERIALIZATION_NVP(portMap);
      ar & BOOST_SERIALIZATION_NVP(ports);
      ar & BOOST_SERIALIZATION_NVP(counters);
      ar & BOOST_SERIALIZATION_NVP(cd);
      ar & BOOST_SERIALIZATION_NVP(debugLevel);
      ar & BOOST_SERIALIZATION_NVP(tog);
      ar & BOOST_SERIALIZATION_NVP(core_id);
      ar & BOOST_SERIALIZATION_NVP(core_count);
      ar & BOOST_SERIALIZATION_NVP(out);
      ar & BOOST_SERIALIZATION_NVP(dumpVcd);
      ar & BOOST_SERIALIZATION_NVP(stopSim);
      ar & BOOST_SERIALIZATION_NVP(registered);
      #endif

      // TODO: Also, somehow, save the CHDL simulator state
    }

    template <class Archive>
      void load(Archive &ar, const unsigned version)
    {
      #if 0
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
      ar & BOOST_SERIALIZATION_NVP(memLink);
      ar & BOOST_SERIALIZATION_NVP(clockFreq);
      ar & BOOST_SERIALIZATION_NVP(netlFile);
      ar & BOOST_SERIALIZATION_NVP(memFile);
      ar & BOOST_SERIALIZATION_NVP(outputBuffer);
      ar & BOOST_SERIALIZATION_NVP(req);
      ar & BOOST_SERIALIZATION_NVP(resp); 
      ar & BOOST_SERIALIZATION_NVP(idMap);
      ar & BOOST_SERIALIZATION_NVP(portMap);
      ar & BOOST_SERIALIZATION_NVP(ports);
      ar & BOOST_SERIALIZATION_NVP(counters);
      ar & BOOST_SERIALIZATION_NVP(cd);
      ar & BOOST_SERIALIZATION_NVP(debugLevel);
      ar & BOOST_SERIALIZATION_NVP(tog);
      ar & BOOST_SERIALIZATION_NVP(core_id);
      ar & BOOST_SERIALIZATION_NVP(core_count);
      ar & BOOST_SERIALIZATION_NVP(out);
      ar & BOOST_SERIALIZATION_NVP(dumpVcd);
      ar & BOOST_SERIALIZATION_NVP(stopSim);
      ar & BOOST_SERIALIZATION_NVP(registered);
      #endif

      // TODO: Also, somehow, load the CHDL simulator state
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

  private:
    void init_io_pre(const std::string &port);
    void init_io(const std::string &port, std::vector<chdl::node> &v);

    void consoleOutput(char c);

    void handleEvent(Interfaces::SimpleMem::Request *);
    bool clockTick(Cycle_t);

    Interfaces::SimpleMem *memLink;

    std::string clockFreq, netlFile, memFile, outputBuffer;

    std::vector<reqdata> req;
    std::vector<respdata> resp;
    std::map<unsigned long, unsigned long> idMap, portMap;
    std::map<std::string, unsigned> ports;
    std::map<std::string, unsigned long *> counters;
    std::vector<std::queue<Interfaces::SimpleMem::Request *> > resp_q;

    chdl::cdomain_handle_t cd;

    int debugLevel, tog, core_id, core_count;

    Output out;
    std::ofstream vcd;
    bool dumpVcd, stopSim, registered;

    std::vector<unsigned> responses_this_cycle;
};

}}

#endif
