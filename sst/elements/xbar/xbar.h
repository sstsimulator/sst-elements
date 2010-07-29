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


#ifndef _XBAR_H
#define _XBAR_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>

using namespace SST;

#if DBG_XBAR
#define _XBAR_DBG( fmt, args...)\
         printf( "%d:Xbar::%s():%d: "fmt, _debug_rank,__FUNCTION__,__LINE__, ## args )
#else
#define _XBAR_DBG( fmt, args...)
#endif

class Xbar : public Component {

    public:
        Xbar( ComponentId_t id, Params_t& params ) :
            Component( id ), 
            params( params ),
            frequency( "2.2GHz" )
        { 
            _XBAR_DBG("new id=%lu\n",id);

	    if ( params.find("clock") != params.end() ) {
		frequency = params["clock"];
	    }

            cpu = configureLink( "port0" );

	    nic = configureLink( "port1", new Event::Handler<Xbar>(this,&Xbar::processEvent) );
	    selfPush = configureSelfLink("selfPush",new Event::Handler<Xbar>(this,&Xbar::selfEvent));
	    selfPull = configureSelfLink("selfPull");
	    
	    TimeConverter* tc = registerClock( frequency, new Clock::Handler<Xbar>(this, &Xbar::clock) );
	    cpu->setDefaultTimeBase(tc);
	    nic->setDefaultTimeBase(tc);
	    selfPush->setDefaultTimeBase(tc);
	    selfPull->setDefaultTimeBase(tc);

            _XBAR_DBG("Done registering clock\n",id);
        }

    private:

        Xbar() : Component(-1) {} // for serialization only
        Xbar( const Xbar& c );
        bool clock( Cycle_t );
        void processEvent( Event*  );
	void selfEvent( Event*);
	
        Params_t    params;
        Link*       cpu;
        Link*       nic;
	Link*       selfPush;
	Link*       selfPull;
	std::string frequency;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    printf("xbaf::serialize()\n");
            _AR_DBG(Xbar,"start\n");
	    printf("  base serializing: Component\n");
	    printf("  serializing: cpu\n");
            ar & BOOST_SERIALIZATION_NVP( cpu );
	    printf("  serializing: nic\n");
            ar & BOOST_SERIALIZATION_NVP( nic );
            _AR_DBG(Xbar,"done\n");
        }
};

#endif
