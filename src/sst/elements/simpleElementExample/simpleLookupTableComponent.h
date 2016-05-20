// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLELOOKUPTABLECOMPONENT_H
#define _SIMPLELOOKUPTABLECOMPONENT_H

#include <sst/core/component.h>
#include <sst/core/output.h>
#include <sst/core/sharedRegion.h>

namespace SST {
namespace SimpleElementExample {

class SimpleLookupTableComponent : public SST::Component
{
public:
    SimpleLookupTableComponent(SST::ComponentId_t id, SST::Params& params);
    ~SimpleLookupTableComponent();

    virtual void init(unsigned int phase);
    virtual void setup();
    virtual void finish();

    bool tick(SST::Cycle_t);
private:
    Output out;
    const uint8_t * table;
    size_t tableSize;
    SharedRegion *sregion;
};

}
}

#endif
