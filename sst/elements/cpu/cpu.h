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


#ifndef _CPU_H
#define _CPU_H

#include <sst/core/eventFunctor.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

using namespace SST;

#if DBG_CPU
#define _CPU_DBG( fmt, args...)\
         printf( "%d:Cpu::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _CPU_DBG( fmt, args...)
#endif

class Cpu : public Component {
        typedef enum { WAIT, SEND } state_t;
        typedef enum { WHO_NIC, WHO_MEM } who_t;
    public:
        Cpu( ComponentId_t id, Params_t& params ) :
            Component( id ),
            params( params ),
            state(SEND),
            who(WHO_MEM), 
            frequency( "2.2GHz" )
        {
            _CPU_DBG( "new id=%lu\n", id );

            registerExit();

            Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _CPU_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("clock") ) {
/*                     sscanf( it->second.c_str(), "%f", &frequency ); */
		    frequency = it->second;
                }    
                ++it;
            } 
            
            mem = LinkAdd( "MEM" );
/*             handler = new EventHandler< Cpu, bool, Cycle_t, Time_t > */
/*                                                 ( this, &Cpu::clock ); */
            handler = new EventHandler< Cpu, bool, Cycle_t >
                                                ( this, &Cpu::clock );
            _CPU_DBG("-->frequency=%s\n",frequency.c_str());
            TimeConverter* tc = registerClock( frequency, handler );
	    printf("CPU period: %ld\n",(long int) tc->getFactor());
            _CPU_DBG("Done registering clock\n");
            
        }
        int Setup() {
            _CPU_DBG("\n");
            return 0;
        }
        int Finish() {
            _CPU_DBG("\n");
            return 0;
        }

    private:

        Cpu( const Cpu& c );
	Cpu() {}

        bool clock( Cycle_t );
        bool handler1( Time_t time, Event *e );

        ClockHandler_t* handler;
        Params_t    params;
        Link*       mem;
        state_t     state;
        who_t       who;
	std::string frequency;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        ar & boost::serialization::base_object<Component>(*this);
        ar & BOOST_SERIALIZATION_NVP(handler);
        ar & BOOST_SERIALIZATION_NVP(params);
        ar & BOOST_SERIALIZATION_NVP(mem);
        ar & BOOST_SERIALIZATION_NVP(state);
        ar & BOOST_SERIALIZATION_NVP(who);
        ar & BOOST_SERIALIZATION_NVP(frequency);
    }
};

#endif
