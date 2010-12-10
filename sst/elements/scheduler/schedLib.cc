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

#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include <sst/core/element.h>

#include "nodeComponent.h"
#include "schedComponent.h"

static SST::Component*
create_schedComponent(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new schedComponent( id, params );
}

static SST::Component*
create_nodeComponent(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new nodeComponent( id, params );
}

static const SST::ElementInfoComponent components[] = {
    { "schedComponent",
      "Schedular Component",
      NULL,
      create_schedComponent
    },
    { "nodeComponent",
      "Component for use with Scheduler",
      NULL,
      create_nodeComponent
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
  SST::ElementLibraryInfo scheduler_eli = {
        "scheduler",
        "High Level Schedular Components",
        components,
    };
}

