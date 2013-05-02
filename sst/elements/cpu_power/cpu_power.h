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

#ifndef _CPU_POWER_H
#define _CPU_POWER_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/introspectedComponent.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include "../power/power.h"

using namespace SST::Power;

namespace SST {
namespace CPU_power {


class Cpu_power : public IntrospectedComponent {
public:

  Cpu_power(ComponentId_t id, Params_t& params);

  
//int Setup() {   // Renamed per Issue 70 - ALevine
void setup() { 
	  // report/register power dissipation	    
	    power = new SST::Power::Power(getId());
	    power->setChip(params);
            power->setTech(getId(), params, CACHE_IL1, McPAT);
	    power->setTech(getId(), params, CACHE_DL1, McPAT);
	    power->setTech(getId(), params, RF, McPAT);
	    power->setTech(getId(), params, IB, McPAT);
	    power->setTech(getId(), params, EXEU_ALU, McPAT); 
	    power->setTech(getId(), params, LSQ, McPAT);	   	   
	    power->setTech(getId(), params, CACHE_L2, McPAT);
	    power->setTech(getId(), params, INST_DECODER, McPAT);	    
	    power->setTech(getId(), params, CLOCK, IntSim);
	    power->setTech(getId(), params, ROUTER, ORION);  
	   
//           return 0;
}

/*int finish() {
       std::pair<bool, Pdissipation_t> res = readPowerStats(this);
	    if(res.first){ 
	        using namespace io_interval; std::cout <<"ID " << getId() <<": current total power = " << res.second.currentPower << " W" << std::endl;
	        using namespace io_interval; std::cout <<"ID " << getId() <<": leakage power = " << res.second.leakagePower << " W" << std::endl;
	        using namespace io_interval; std::cout <<"ID " << getId() <<": runtime power = " << res.second.runtimeDynamicPower << " W" << std::endl;
	        using namespace io_interval; std::cout <<"ID " << getId() <<": TDP = " << res.second.TDP << " W" << std::endl;
	        using namespace io_interval; std::cout <<"ID " << getId() <<": total energy = " << res.second.totalEnergy << " J" << std::endl;
	        using namespace io_interval; std::cout <<"ID " << getId() <<": peak power = " << res.second.peak << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L2 current total power = " << res.second.itemizedCurrentPower.L2 << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L2 leakage power = " << res.second.itemizedLeakagePower.L2 << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L2 runtime power = " << res.second.itemizedRuntimeDynamicPower.L2 << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L3 current total power = " << res.second.itemizedCurrentPower.L3 << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L3 leakage power = " << res.second.itemizedLeakagePower.L3 << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L3 runtime power = " << res.second.itemizedRuntimeDynamicPower.L3 << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L1dir current total power = " << res.second.itemizedCurrentPower.L1dir << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L1dir leakage power = " << res.second.itemizedLeakagePower.L1dir << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L1dir runtime power = " << res.second.itemizedRuntimeDynamicPower.L1dir << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": router current total power = " << res.second.itemizedCurrentPower.router << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": router leakage power = " << res.second.itemizedLeakagePower.router << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": router runtime power = " << res.second.itemizedRuntimeDynamicPower.router << " W" << std::endl;
	        using namespace io_interval; std::cout <<"ID " << getId() <<": current sime time = " << res.second.currentSimTime << " second" << std::endl;
		power->printFloorplanAreaInfo();
		std::cout << "area return from McPAT = " << power->estimateAreaMcPAT() << " mm^2" << std::endl;
		power->printFloorplanPowerInfo();
		power->printFloorplanThermalInfo();
	   }

    static int n = 0;
    n++;
    if (n == 10) {
      printf("Several Simple Components Finished\n");
    } else if (n > 10) {
      ;
    } else {
      printf("Simple Component for Power and Thermal Simulations Finished\n");
    }

      
            return 0;
        }*/
// int Finish() {  // Renamed per Issue 70 - ALevine
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
//    return 0;
  }


public:
  private:
  Cpu_power();  // for serialization only
  Cpu_power(const Cpu_power&); // do not implement
  void operator=(const Cpu_power&); // do not implement

  void handleEvent( Event *ev );
  virtual bool clockTic( Cycle_t );
  
  int workPerCycle;
  int commFreq;
  int commSize;
  int neighbor;

  Link* N;
  Link* S;
  Link* E;
  Link* W;

  Params_t    params;
  Pdissipation_t pdata, pstats;
  SST::Power::Power *power;
  usagecounts_t mycounts;  //over-specified struct that holds usage counts of its sub-components


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
    N->setFunctor(new Event::Handler<Cpu_power>(this,&Cpu_power::handleEvent));
    S->setFunctor(new Event::Handler<Cpu_power>(this,&Cpu_power::handleEvent));
    E->setFunctor(new Event::Handler<Cpu_power>(this,&Cpu_power::handleEvent));
    W->setFunctor(new Event::Handler<Cpu_power>(this,&Cpu_power::handleEvent));
  }
    
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

}
}
#endif /* _SIMPLECOMPONENT_H */
