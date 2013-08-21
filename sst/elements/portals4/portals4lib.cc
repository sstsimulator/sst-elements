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


#include <sst_config.h>
#include <sst/core/serialization.h>

#include <sst/core/element.h>

#include <ptlNic/ptlNic.h>

using namespace SST;

static Component*
create_PtlNic(ComponentId_t id, Component::Params_t& params)
{
    return new SST::Portals4::PtlNic( id, params );
}

static const ElementInfoComponent components[] = {
    { "PtlNic",
      "PtlNic, Portals4 memory mapped nic",
      NULL,
      create_PtlNic
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo portals4_eli = {
        "portals4",
        "portals4",
        components,
    };
}
