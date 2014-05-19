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

#include <sst_config.h>
#include "m5.h"

#include <sst/core/serialization.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/linkMap.h>
#include <sst/core/params.h>
#include <sst/core/interfaces/simpleMem.h>

#include <dll/gem5dll.hh>
#include <sim/simulate.hh>
#include "portLink.h"
#include "util.h"
#include "debug.h"


#define curTick libgem5::CurTick

using namespace SST;
using namespace SST::M5;

SST::M5::M5::M5( ComponentId_t id, Params& params ) :
    IntrospectedComponent( id ),
    m_numRegisterExits( 0 ),
    m_barrier( NULL ),
    params( params )  //for power
{
    libgem5::SetWantInfo( false );
    libgem5::SetRemoteGDBPort(0);

    // a flag for telling if fastforwarding is used 
    FastForwarding_flag=false;

    if ( params.find( "debug" ) != params.end() ) {
        int level = strtol( params["debug"].c_str(), NULL, 0 );
       
        if ( level ) { 
            _dbg.level( level );
        }
    }
    if ( params.find( "M5debug" ) != params.end() ) {
        enableDebug( params[ "M5debug" ] );
    }
    //if ( params.find( "M5debugFile" ) != params.end() ) {
    //    libgem5::SetLogFile( params[ "M5debugFile" ] );
    //}

    if ( params.find( "info" ) != params.end() ) {
        if ( params["info"].compare("yes") == 0 ) {
            _info.enable();
        } 
    }

    std::string configFile = "";
    if ( params.find( "configFile" ) != params.end() ) {
      configFile = params["configFile"];
    }

    m_statFile = params.find_string("statFile", "");
	m_init_link_name = params.find_string("mem_initializer_port");

    /* Gem5's preferred resolution is 1ps */
    unsigned long m5_freq = 1000000000000;
    libgem5::SetClockFrequency( m5_freq );

    INFO( "configFile `%s`\n", configFile.c_str() );
    m_objectMap = buildConfig( this, "m5", configFile, params );

    std::string clock_freq = params.find_string("frequency", "1 GHz");
    TimeConverter *tc = registerClock( clock_freq, new SST::Clock::Handler< M5 >( this, &M5::clock ) );

    TimeConverter *g5tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter("1 ps");

    SimTime_t scale = g5tc->convertFromCoreTime(tc->getFactor());

    m_m5ticksPerSSTclock = scale;
    DBGX(3, "m_m5ticksPerSSTClock = %d\n", m_m5ticksPerSSTclock);


    if ( ! params.find_string("registerExit", "yes").compare("yes") ) {
        INFO("registering exit\n");
        IntrospectedComponent::registerAsPrimaryComponent();
        IntrospectedComponent::primaryComponentDoNotEndSim();
    }

    int numBarrier = params.find_integer( "numBarrier" );
    if ( numBarrier > 0 ) {
        m_barrier = new BarrierAction( numBarrier );
    }

    //
    // for power and thermal modeling
    //
    #ifdef M5_WITH_POWER
    Init_Power();
    #endif
}

SST::M5::M5::~M5()
{
}


void SST::M5::M5::init(unsigned int phase)
{
	if ( !phase ) {
		DBGX( 2, "call initAllObjects()\n" );
		libgem5::InitAllObjects();
		DBGX( 2, "initAllObjects\n" );

		/* Because there's no clean way to get my link (that was configured elsewhere... */
		if ( m_init_link_name != "" ) {
            Interfaces::SimpleMem *memInitLink = m_init_link->getSSTInterface();
			/* TODO: Should be part of intitialization
			 * Also, the call to InitAllObjects will need to be done
			 * before this call. */
			std::vector<libgem5::Blob> blobs;
			libgem5::getInitializedMemory(blobs);

			/* blobs -> memInitLink */
			for ( std::vector<libgem5::Blob>::iterator i = blobs.begin() ; i != blobs.end() ; ++i ) {
				Interfaces::SimpleMem::Request *ev = new Interfaces::SimpleMem::Request(Interfaces::SimpleMem::Request::Write, i->address, i->size);
				ev->setPayload(i->data, i->size);

				memInitLink->sendInitData(ev);

				/* Save memory */
				free(i->data);
				i->data = NULL;
				i->size = 0;
			}
		}

	}

    portLinkDbg = new Output();
    portLinkDbg->init("", 6, 0, (Output::output_location_t)params.find_integer("memory_trace", 0));

	/* Setup Links */
	for ( objectMap_t::iterator i = m_objectMap.begin() ; i != m_objectMap.end() ; ++i ) {
		Gem5Object_t *obj = i->second;
		for ( std::deque<void*>::iterator j = obj->links.begin() ; j != obj->links.end() ; ++j ) {
			PortLink *link = static_cast<PortLink*>(*j);
			link->setup();
            link->setOutput(portLinkDbg);
		}
	}
}

void SST::M5::M5::setup()
{

    #ifdef M5_WITH_POWER
    Setup_Power();
    #endif

}

void SST::M5::M5::registerExit(void)
{
    DBGX(2,"m_numRegisterExits %d\n",m_numRegisterExits);
    if ( m_numRegisterExits == 0) {
      IntrospectedComponent::registerAsPrimaryComponent();
      IntrospectedComponent::primaryComponentDoNotEndSim();
    } 
    ++m_numRegisterExits;
}

void SST::M5::M5::exit( int status )
{
    DBGX(2,"M5::exit() status=%d\n",status);
    --m_numRegisterExits;
    if ( m_numRegisterExits == 0) {
        libgem5::SchedExitEvent();
    } 
}

bool SST::M5::M5::clock( SST::Cycle_t cycle )
{
    DBGX( 5, "current_cycle=%lu\n", cycle * m_m5ticksPerSSTclock );
    DBGX( 5, "call simulate M5 curTick()=%lu\n", curTick() );
    ::SimLoopExitEvent* exitEvent = simulate( m_m5ticksPerSSTclock );
    DBGX( 5, "simulate returned M5 curTick() %lu\n", curTick() );
    
    
    
    /*
    if((cycle * m_m5ticksPerSSTclock) % 100000 == 0){
        for ( objectMap_t::iterator i = m_objectMap.begin() ; i != m_objectMap.end() ; ++i ) {
            Gem5Object_t *obj = i->second;
            for ( std::deque<void*>::iterator j = obj->links.begin() ; j != obj->links.end() ; ++j ) {
                PortLink *link = static_cast<PortLink*>(*j);
                link->printQueueSize();
            }
        }
    }
     */
    
    if( exitEvent->getCode() != 256 )  {
        // for fast-forwarding to reache the ROI at a faster speed in real 
        // benchmarks the exitcode is set to 100 to distinguish from regular 
        // exit
        if ( exitEvent->getCode() == 100 ){
	        //std::cout << "I entered getCode = 100" << std::endl;

	        FastForwarding_flag=true;
#if 0
	        SimObject::SST_FF();
#endif
	        if(simulate()->getCode()>=0){
            primaryComponentOKToEndSim();
	        }
        } else {
            // bug what if we didn't call registerExit()
            primaryComponentOKToEndSim();
            INFO( "exiting: curTick()=%lu cause=`%s` code=%d\n", curTick(),
                exitEvent->getCause().c_str(), exitEvent->getCode() );
        }
        if ( !m_statFile.empty() ) {
            libgem5::DumpStats(m_statFile);
        }
        return true;
    }
    return false;
}

void SST::M5::M5::finish()
{
#ifdef M5_WITH_POWER
    Finish_Power();
#endif

    delete m_barrier;

    if ( !m_statFile.empty() ) {
        libgem5::DumpStats(m_statFile);
    }
}



void SST::M5::M5::registerPortLink(const std::string &name, PortLink* pl)
{
    if ( !name.compare(m_init_link_name) ) {
        m_init_link = pl;
    }
}
