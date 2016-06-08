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

#ifndef _SIMPLECOMPONENT_H
#define _SIMPLECOMPONENT_H

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/rng/marsaglia.h>

namespace SST {
namespace SimpleComponent {

class simpleComponent : public SST::Component 
{
public:
    simpleComponent(SST::ComponentId_t id, SST::Params& params);
    ~simpleComponent();

    void setup() { }
    void finish() {
    	printf("Component Finished.\n");
    }

private:
    simpleComponent();  // for serialization only
    simpleComponent(const simpleComponent&); // do not implement
    void operator=(const simpleComponent&); // do not implement

    void handleEvent(SST::Event *ev);
    virtual bool clockTic(SST::Cycle_t);

    int workPerCycle;
    int commFreq;
    int commSize;
    int neighbor;

    SST::RNG::MarsagliaRNG* rng;
    SST::Link* N;
    SST::Link* S;
    SST::Link* E;
    SST::Link* W;
};

} // namespace SimpleComponent
} // namespace SST

#endif /* _SIMPLECOMPONENT_H */
