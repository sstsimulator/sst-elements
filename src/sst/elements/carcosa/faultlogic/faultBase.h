// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_ELEMENTS_CARCOSA_FAULTBASE_H
#define SST_ELEMENTS_CARCOSA_FAULTBASE_H

#include "sst/core/component.h"
#include "sst/core/event.h"
#include "sst/elements/memHierarchy/memEvent.h"

namespace SST::Carcosa {

    /** TODO:
     * Parameters for switching between interface-driven and normal instantiation
     * A way to read in data to choose which logic to use
     *  - Might be possible to parameterize the entirety of the logic, but would be easier if I can 
     *    build a "library" of functions that are loaded dynamically
    */

class FaultBase {
public:
    FaultBase(Params& params);

    FaultBase() = default;
    ~FaultBase() {}

    virtual void faultLogic(Event*& ev);
};
}

#endif