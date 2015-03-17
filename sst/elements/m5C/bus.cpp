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

#include <mem/bus.hh>

#include <debug.h>
#include <paramHelp.h>

using namespace SST;
using namespace SST::M5;
using namespace std;

class Component;

extern "C" {
SimObject* create_Bus( Component*, string name, Params& params );
}

SimObject* create_Bus( Component*, string name, Params& params )
{
    BusParams& busP             = * new BusParams;

    busP.name = name;

    INIT_CLOCK( busP, params, clock);
    INIT_INT( busP, params, block_size);
    INIT_INT( busP, params, bus_id );
    INIT_INT( busP, params, header_cycles);
    INIT_INT( busP, params, width);

    return busP.create();
}
