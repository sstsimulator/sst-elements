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

#include <physicalPlus.h>
#include <memLink.h>
#include <paramHelp.h>
#ifdef fatal
#undef fatal
#endif
#include <loadMemory.h>


using namespace SST::M5;

extern "C" {
    SimObject* create_PhysicalPlus( SST::Component*, std::string name,
                                                    SST::Params& params );
}

SimObject* create_PhysicalPlus( SST::Component* comp, std::string name,
                                                    SST::Params& _params )
{
    PhysicalMemoryPlusParams& params = *new PhysicalMemoryPlusParams;

    params.name = name;

    INIT_HEX( params, _params, range.start );
    INIT_HEX( params, _params, range.end );
    INIT_INT( params, _params, latency );
    INIT_INT( params, _params, latency_var );
    INIT_BOOL( params, _params, null );
    INIT_BOOL( params, _params, zero );
    INIT_STR( params, _params, file );

    params.linkName = _params.find_string( "link.name" );
    params.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );

    ::PhysicalMemory* physmem = new PhysicalMemoryPlus( &params );

    loadMemory( name + ".exe", physmem, _params.find_prefix_params("exe") );

    return physmem;
}

PhysicalMemoryPlus::PhysicalMemoryPlus( const PhysicalMemoryPlusParams *p ) :
    PhysicalMemory( p )
{
    DBGX( 2, "name=`%s`\n", name().c_str() );
    Port* port = getPort( "port", 0 );
    assert( port );

    SST::Params params; 
    params["name"]        = p->linkName;
    params["range.start"] = p->range.start;
    params["range.end"]   = p->range.end;

    m_link =  MemLink::create( name(), p->m5Comp, port, params );
}
