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

#include "sst_config.h"

#include "pyproto.h"

#include "sst/core/element.h"

extern "C" void* genPyProtoPyModule(void);

using namespace SST;

static Component* create_pyproto(SST::ComponentId_t id, SST::Params& params)
{
    return new PyProtoNS::PyProto(id, params);
}


static const ElementInfoParam pyProto_params[] = {
    { NULL, NULL, NULL}
};

static const ElementInfoPort pyProto_ports[] = {
    {"port%d", "Link to another object", NULL},
    {NULL, NULL, NULL}
};


static const ElementInfoComponent pyProto_Components[] = {
    { "PyProto",                                 // Name
      "Python Prototyping Component",            // Description
      NULL,                                      // PrintHelp
      create_pyproto,                            // Allocator
      pyProto_params,                            // Parameters
      pyProto_ports,                             // Ports
      COMPONENT_CATEGORY_UNCATEGORIZED,          // Category
      NULL                                       // Statistics
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL}
};

extern "C" {
    ElementLibraryInfo pyproto_eli = {
        "pyproto",                          // Name
        "Python-based Prototyping Library", // Description
        pyProto_Components,                 // Components
        NULL,                               // Events
        NULL,                               // Introspectors
        NULL,                               // Modules
        NULL,                               // Subcomponents
        NULL,                               // Partitioners
        genPyProtoPyModule,                 // Python Module Generator
        NULL                                // Generators
    };
}


