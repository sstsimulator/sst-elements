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

// A cpu component that can have statistics introspected 
// Built for testing the introspector.

#ifndef _CPU_DATA_H
#define _CPU_DATA_H

#include <sst/core/eventFunctor.h>
#include <sst/core/component.h>
#include <sst/core/link.h>


using namespace SST;

#if DBG_CPU_DATA
#define _CPU_DATA_DBG( fmt, args...)\
         printf( "%d:Cpu_data::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _CPU_DATA_DBG( fmt, args...)
#endif

class Cpu_data : public Component {
        typedef enum { WAIT, SEND } state_t;
        typedef enum { WHO_NIC, WHO_MEM } who_t;
    public:
        Cpu_data( ComponentId_t id, Params_t& params ) :
            Component( id ),
            state(SEND),
            who(WHO_MEM), 
            frequency( "2.2GHz" )
        {
            _CPU_DATA_DBG( "new id=%lu\n", id );

	    registerExit();

            Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _CPU_DATA_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("clock") ) {
                    frequency = it->second;
                }    
		else if ( ! it->first.compare("push_introspector") ) {
                    pushIntrospector = it->second;
                }    

                ++it;
            } 
            
            mem = LinkAdd( "MEM" );
            handler = new EventHandler< Cpu_data, bool, Cycle_t >
                                                ( this, &Cpu_data::clock );
	    handlerPush = new EventHandler< Cpu_data, bool, Cycle_t >
                                                ( this, &Cpu_data::pushData );

            TimeConverter* tc = registerClock( frequency, handler );
	    registerClock( frequency, handlerPush );

	    printf("CPU_DATA period: %ld\n", (long int)tc->getFactor());
            _CPU_DATA_DBG("Done registering clock\n");

	    counts = 0;
	    num_il1_read = 0;
	    num_branch_read = 0;
	    num_branch_write = 0;
	    num_RAS_read = 0;
	    num_RAS_write = 0;

	    registerMonitorInt("il1_read");
	    registerMonitorInt("core_temperature");
	    registerMonitorInt("branch_read");
	    registerMonitorInt("RAS_read");
	    registerMonitorInt("RAS_write");
	    

        }
        int Setup() {   
           return 0;
        }
        int Finish() {
            _CPU_DATA_DBG("\n");
	    unregisterExit();
            return 0;
        }
	
	uint64_t getIntData(int dataID, int index=0)
	{ 
	  switch(dataID)
	  {
	    case 0:
	    //core_temperature
		return (mycore_temperature);
		break;
	    case 1:
	    //branch_read
		return (num_branch_read);
		break;
	    case 2: 
	    //branch_write
		return (num_branch_write);
		break;
	    case 3: 
	    //RAS_read 
		return (num_RAS_read);
		break;
	    case 4:
	    //RAS_write
		return (num_RAS_write);
		break;
	    case 5:
	    //il1_read
		return (num_il1_read);
		break;
	    default:
		return (0);
		break;	
	  }
	}
	

    public:
	int counts;
	uint64_t num_il1_read;
	uint64_t mycore_temperature;
	uint64_t num_branch_read;
	uint64_t num_branch_write;
	uint64_t num_RAS_read;
	uint64_t num_RAS_write;

    private:
        Cpu_data();
        Cpu_data( const Cpu_data& c );

        bool clock( Cycle_t );
	bool pushData( Cycle_t);
        ClockHandler_t *handler, *handlerPush;
        bool handler1( Time_t time, Event *e );

        Link*       mem;
        state_t     state;
        who_t       who;
	std::string frequency;
	std::string pushIntrospector;
};

#endif
