// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <Python.h>
#pragma clang diagnostic pop

#include <vector>

#include <sst/core/event.h>
#include <sst/core/serialization/serialize.h>

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
#if 0
        PyEvent_t *out = NULL;
        polymorphic_PyEvent_oarchive oa(std::cout, boost::archive::no_header|boost::archive::no_codecvt);

        oa << event;

        return oa.getEvent();
#endif
    }
    return NULL;
}


}
}
