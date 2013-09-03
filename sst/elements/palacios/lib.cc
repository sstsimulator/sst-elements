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
#include <sst/core/params.h>

#include "ptlNicMMIF.h"

using namespace SST;
using namespace SST::Palacios;

static Component*
create_PtlNicMMIF(ComponentId_t id, Params& params)
{
    return new PtlNicMMIF( id, params );
}

static const ElementInfoComponent components[] = {
    { "PtlNicMMIF",
      "PtlNicMMIF Component",
      NULL,
      create_PtlNicMMIF
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo palacios_eli = {
        "PtlNicMMIF",
        "PtlNicMMIF Component",
        components,
    };
}

