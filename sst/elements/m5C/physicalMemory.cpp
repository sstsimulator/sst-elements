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
#include <sst/core/params.h>
#include <sst/core/output.h> // Must be loaded before any Gem5 includes (overwrites 'fatal')

#include <mem/physical.hh>

#include <debug.h>
#include <paramHelp.h>
#include <loadMemory.h>

using namespace SST;
using namespace SST::M5;
using namespace std;

class Component;

extern "C" {
  SimObject* create_PhysicalMemory( Component*, string name, Params& sParams );
}

SimObject* create_PhysicalMemory( Component*, string name, Params& sParams )
{
    PhysicalMemoryParams& params   = *new PhysicalMemoryParams;

    params.name = name;

    INIT_HEX( params, sParams, range.start );
    INIT_HEX( params, sParams, range.end );
    INIT_LATENCY( params, sParams, latency );
    INIT_LATENCY( params, sParams, latency_var );
    INIT_BOOL( params, sParams, null );
    INIT_BOOL( params, sParams, zero );
    INIT_STR( params, sParams, file );
    INIT_STR( params, sParams, ddrConfig );
    INIT_INT( params, sParams, tx_q );

    params.megsMem = params.range.size() / (1024 * 1024);
    PhysicalMemory* physmem = params.create();

    loadMemory( name + ".exe", physmem, sParams.find_prefix_params("exe") );

    return physmem;
}
