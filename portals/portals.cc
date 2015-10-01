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

#include <sst/core/component.h>
#include <sst/core/element.h>
#include <sst/core/params.h>

using namespace SST;

static SST::Component*
create_portals4_driver(SST::ComponentId_t id,
                       SST::Params& params)
{
    printf("driver called\n");
    return NULL;
}

static const ElementInfoComponent components[] = {
    { "4.driver",
      "State machine based driver for Portals nics",
      NULL,
      create_portals4_driver
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo portals_eli = {
        "portals",
        "portals",
        components,
    };
}
