// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// A cpu component that can report unit power 
// Built for testing the power model.

#ifndef _CPU_POWER_H
#define _CPU_POWER_H

//#include <sst/core/eventFunctor.h>
#include <sst/core/introspectedComponent.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include "../power/power.h"

using namespace SST;

#if DBG_CPU_POWER
#define _CPU_POWER_DBG( fmt, args...)\
         printf( "%d:Cpu_power::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _CPU_POWER_DBG( fmt, args...)
#endif

class Cpu_power : public IntrospectedComponent {
        typedef enum { WAIT, SEND } state_t;
        typedef enum { WHO_NIC, WHO_MEM } who_t;
    public:
        Cpu_power( ComponentId_t id, Params_t& params ) :
            IntrospectedComponent( id ),
            params( params ),
            state(SEND),
            who(WHO_MEM), 
            frequency( "2.2GHz" )
        {
            _CPU_POWER_DBG( "new id=%lu\n", id );

	    registerExit();

            Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _CPU_POWER_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("clock") ) {
                    frequency = it->second;
                }    
                ++it;
            } 
            
            mem = configureLink( "MEM" );
 //           handler = new SST::EventHandler< Cpu_power, bool, Cycle_t >
 //                                               ( this, &Cpu_power::clock );
 //           TimeConverter* tc = registerClock( frequency, handler );
       TimeConverter* tc = registerClock( frequency, new Clock::Handler<Cpu_power>(this,&Cpu_power::clock) );
	  
	    mem->setDefaultTimeBase(tc);
	    printf("CPU_POWER period: %ld\n",tc->getFactor());
            _CPU_POWER_DBG("Done registering clock\n");

	    
        }
        int Setup() {
            // report/register power dissipation	    
	    power = new Power(getId());
	    power->setChip(params);
            power->setTech(getId(), params, CACHE_IL1, McPAT);
	    power->setTech(getId(), params, CACHE_DL1, McPAT);
	    power->setTech(getId(), params, CACHE_ITLB, McPAT);
	    power->setTech(getId(), params, CACHE_DTLB, McPAT);
	    power->setTech(getId(), params, RF, McPAT);
	    power->setTech(getId(), params, IB, McPAT);
	    power->setTech(getId(), params, PIPELINE, McPAT);
	    power->setTech(getId(), params, BYPASS, McPAT);
	    //power->setTech(getId(), params, EXEU_ALU, McPAT);
	    //power->setTech(getId(), params, EXEU_FPU, McPAT);
	    power->setTech(getId(), params, LSQ, McPAT);
	    power->setTech(getId(), params, BPRED, McPAT);
	    power->setTech(getId(), params, SCHEDULER_U, McPAT);
	    power->setTech(getId(), params, RENAME_U, McPAT);
	    //power->setTech(getId(), params, BTB, McPAT);
	    power->setTech(getId(), params,  LOAD_Q, McPAT);
	    power->setTech(getId(), params, CACHE_L1DIR, McPAT);
	    power->setTech(getId(), params, CACHE_L2DIR, McPAT);
	    power->setTech(getId(), params, CACHE_L2, McPAT);
	    power->setTech(getId(), params, CACHE_L3, McPAT);
	    //power->setTech(getId(), params, MEM_CTRL, McPAT);
	    power->setTech(getId(), params, ROUTER, McPAT);
           return 0;
        }
        int Finish() {
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
		using namespace io_interval; std::cout <<"ID " << getId() <<": L2dir current total power = " << res.second.itemizedCurrentPower.L2dir << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L2dir leakage power = " << res.second.itemizedLeakagePower.L2dir << " W" << std::endl;
		using namespace io_interval; std::cout <<"ID " << getId() <<": L2dir runtime power = " << res.second.itemizedRuntimeDynamicPower.L2dir << " W" << std::endl;
	        using namespace io_interval; std::cout <<"ID " << getId() <<": current sime time = " << res.second.currentSimTime << " second" << std::endl;
		power->printFloorplanAreaInfo();
		std::cout << "area return from McPAT = " << power->estimateAreaMcPAT() << " mm^2" << std::endl;
		power->printFloorplanPowerInfo();
		power->printFloorplanThermalInfo();
	    }
            _CPU_POWER_DBG("\n");
	    //unregisterExit();
            return 0;
        }


    private:

        Cpu_power( const Cpu_power& c );
	Cpu_power() :  IntrospectedComponent(-1) {} // for serialization only

        bool clock( Cycle_t );
        //bool handler1( Time_t time, Event *e );

        Params_t    params;
        Link*       mem;
        state_t     state;
        who_t       who;
	std::string frequency;
	

	Pdissipation_t pdata, pstats;
	Power *power;
	usagecounts_t mycounts;  //over-specified struct that holds usage counts of its sub-components


	  friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(params);
        ar & BOOST_SERIALIZATION_NVP(mem);
        ar & BOOST_SERIALIZATION_NVP(state);
        ar & BOOST_SERIALIZATION_NVP(who);
        ar & BOOST_SERIALIZATION_NVP(frequency);
    }
};

#endif
