// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _CPU_DATA_H
#define _CPU_DATA_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/introspectedComponent.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

namespace SST {
namespace CPU_data {

class Cpu_data : public SST::IntrospectedComponent {
public:

  Cpu_data(SST::ComponentId_t id, SST::Component::Params_t& params);

  void finish() {
    static int n = 0;
    n++;
    if (n == 10) {
      printf("Several Simple Components Finished\n");
    } else if (n > 10) {
      ;
    } else {
      printf("Simple Component Finished\n");
    }
  }

void setup() {   
	   registerIntrospector(pushIntrospector);
	   registerMonitor("totalCounts", new MonitorFunction<Cpu_data, uint64_t>(this, &Cpu_data::someCalculation));
	   registerMonitor("temperature", new MonitorFunction<Cpu_data, double>(this, &Cpu_data::updateTemperature));
	   registerMonitor("generalCounts", new MonitorPointer<int>(&counts));
}

public:
  int counts;
	double mycore_temperature;
	uint64_t num_il1_read;
	uint64_t num_branch_read;
	uint64_t num_branch_write;
	uint64_t num_RAS_read;
	uint64_t num_RAS_write;
private:
  Cpu_data();  // for serialization only
  Cpu_data(const Cpu_data&); // do not implement
  void operator=(const Cpu_data&); // do not implement

  void handleEvent( SST::Event *ev );
  virtual bool clockTic( SST::Cycle_t );
  uint64_t someCalculation();
  double updateTemperature();
  std::string pushIntrospector;

  int workPerCycle;
  int commFreq;
  int commSize;
  int neighbor;

  SST::Link* N;
  SST::Link* S;
  SST::Link* E;
  SST::Link* W;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(workPerCycle);
    ar & BOOST_SERIALIZATION_NVP(commFreq);
    ar & BOOST_SERIALIZATION_NVP(commSize);
    ar & BOOST_SERIALIZATION_NVP(neighbor);
    ar & BOOST_SERIALIZATION_NVP(N);
    ar & BOOST_SERIALIZATION_NVP(S);
    ar & BOOST_SERIALIZATION_NVP(E);
    ar & BOOST_SERIALIZATION_NVP(W);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(workPerCycle);
    ar & BOOST_SERIALIZATION_NVP(commFreq);
    ar & BOOST_SERIALIZATION_NVP(commSize);
    ar & BOOST_SERIALIZATION_NVP(neighbor);
    ar & BOOST_SERIALIZATION_NVP(N);
    ar & BOOST_SERIALIZATION_NVP(S);
    ar & BOOST_SERIALIZATION_NVP(E);
    ar & BOOST_SERIALIZATION_NVP(W);
    //resture links
    N->setFunctor(new SST::Event::Handler<Cpu_data>(this,&Cpu_data::handleEvent));
    S->setFunctor(new SST::Event::Handler<Cpu_data>(this,&Cpu_data::handleEvent));
    E->setFunctor(new SST::Event::Handler<Cpu_data>(this,&Cpu_data::handleEvent));
    W->setFunctor(new SST::Event::Handler<Cpu_data>(this,&Cpu_data::handleEvent));
  }
    
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

}
}
#endif /* _SIMPLECOMPONENT_H */
