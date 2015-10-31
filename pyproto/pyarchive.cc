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

#include "pymodule.h"
#include "pyproto.h"

#include "pyarchive.h"

namespace SST {
namespace PyProtoNS {

PyEvent_t *convertEventToPython(SST::Event *event)
{
    PyEvent *pe = dynamic_cast<PyEvent*>(event);
    if ( pe ) {
        return pe->getPyObj();
    } else {
        PyEvent_t *out = NULL;
        polymorphic_PyEvent_oarchive oa(std::cout, boost::archive::no_header|boost::archive::no_codecvt);

        oa << event;

        return oa.getEvent();
    }
    return NULL;
}


}
}
