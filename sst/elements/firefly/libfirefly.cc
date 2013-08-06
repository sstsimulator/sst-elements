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


#include "sst_config.h"
#include <sst/core/serialization.h>

#include "sst/core/element.h"

#include <ioSwitch.h>
#include <nic.h>
#include <testDriver.h>
#include <hades.h>
#include <simpleIO.h>
#if 0
#include <merlinIO.h>
#endif

using namespace Firefly;

static SST::Component*
create_ioSwitch(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new IOSwitch( id, params );
}

static SST::Component*
create_nic(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new Nic( id, params );
}

static SST::Component*
create_testDriver(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new TestDriver(id, params);
}

static Module*
load_hades(Params& params)
{
    return new Hades(params);
}

static Module*
load_simpleIO(Params& params)
{
    return new SimpleIO(params);
}

#if 0
static Module*
load_merlinIO(Params& params)
{
    return new MerlinIO(params);
}
#endif

static const ElementInfoComponent components[] = {
    { "nic",
      "Firefly NIC",
      NULL,
      create_nic,
    },
    { "ioSwitch",
      "Firefly IO Switch",
      NULL,
      create_ioSwitch,
    },
    { "testDriver",
      "Firefly test driver ",
      NULL,
      create_testDriver,
    },
    { NULL, NULL, NULL, NULL }
};

static const ElementInfoModule modules[] = {
    { "hades",
      "Firefly Hermes module",
      NULL,
      load_hades,
      NULL,
    },
    { "testIO",
      "Firefly IO module",
      NULL,
      load_simpleIO,
      NULL,
    },
#if 0
    { "merlinIO",
      "Merlin IO module",
      NULL,
      load_merlinIO,
      NULL,
    },
#endif
    { NULL, NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo firefly_eli = {
        "nic",
        "Firefly NIC",
        components,
        NULL,
        NULL,
        modules,
        NULL,
        NULL,
    };
}
