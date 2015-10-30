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
#include <Python.h>

#include <vector>

#include <sst/core/event.h>
#include <sst/core/interfaces/stringEvent.h>

#include "pymodule.h"
#include "pyproto.h"

namespace SST {
namespace PyProtoNS {

PyEvent_t *convertEventToPython(SST::Event *event)
{
    PyEvent *pe = dynamic_cast<PyEvent*>(event);
    if ( pe ) {
        return pe->getPyObj();
    } else if ( Interfaces::StringEvent *se = dynamic_cast<Interfaces::StringEvent*>(event) ) {
        //PyEvent_t *res = 
    }
    return NULL;
}


}
}
