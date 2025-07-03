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

class FaultBase {
public:
    FaultBase(Params& params);

    FaultBase() = default;
    ~FaultBase() {}
    
    virtual void faultLogic(Event*& ev) = 0;
};
}

#endif