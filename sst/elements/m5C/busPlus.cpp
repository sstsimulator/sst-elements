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

#include <sst_config.h>
#include <sst/core/serialization.h>

#include <busPlus.h>
#include <memLink.h>
#include <paramHelp.h>

#include <sst/core/params.h>

using namespace SST::M5;

extern "C" {
    SimObject* create_BusPlus( SST::Component*, std::string name, 
                                                    SST::Params& params );
}

SimObject* create_BusPlus( SST::Component* comp, std::string name,
                                                    SST::Params& params )
{
    BusPlusParams& busP = *new BusPlusParams;

    busP.name = name;

    INIT_CLOCK( busP, params, clock);
    INIT_INT( busP, params, block_size);
    INIT_INT( busP, params, bus_id );
    INIT_INT( busP, params, header_cycles);
    INIT_INT( busP, params, width);
//    INIT_BOOL( busP, params, async);

    busP.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );
    busP.params = params.find_prefix_params( "link." );

    return new BusPlus( &busP );
}

BusPlus::BusPlus( const BusPlusParams *p ) : 
    Bus( p )
{
    DBGX( 2, "name=`%s`\n", name().c_str() );

    if ( p->params.empty() ) {
        return;
    }

    int num = 0;
    while ( 1 ) {
        std::stringstream numSS;
        std::string tmp;

        numSS << num;

        tmp = numSS.str() + ".";

        SST::Params params = p->params.find_prefix_params( tmp );

        if ( params.empty() ) {
            return;
        }

        Port* port = getPort( "port" );
        assert( port );

        m_links.push_back( MemLink::create( name(), p->m5Comp, port, params ) );
        ++num;
    }
}
