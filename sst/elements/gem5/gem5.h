// Copyright 2009-2015 Sandia Coporation.  Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package.  For lucense
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_GEM5_GEM5_H
#define SST_GEM5_GEM5_H

#include <string>
#include <vector>

#include <sst/core/serialization.h>
#include <sst/core/component.h>
#include <sst/core/output.h>

#include "Gem5Connector.h"

class System;  // Gem5 System object

namespace SST {
namespace gem5 {

class Gem5Comp : public SST::Component {
private:

    Output dbg;
    Output info;
    uint64_t sim_cycles;
    uint64_t clocks_processed;

    std::vector<Gem5Connector*> connectors;
    System* g5system;

    void splitCommandArgs(std::string &cmd, std::vector<char *> &args);
    void splitConnectors(std::string &cmd, std::vector<std::string> &ports);
    void initPython( int argc, char *argv[] );

public:
    Gem5Comp(ComponentId_t id, Params &params);
    ~Gem5Comp();
    virtual void init(unsigned int);
    virtual void setup();
    virtual void finish();
    bool clockTick(Cycle_t);
};

}
}


#endif   /* ifndef SST_GEM5_GEM5_H */
