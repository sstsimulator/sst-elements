// -*- mode: c++ -*-

// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_TARGET_GENERATOR_TARGET_GENERATOR_H
#define COMPONENTS_MERLIN_TARGET_GENERATOR_TARGET_GENERATOR_H

#include <sst/core/subcomponent.h>

namespace SST {
namespace Merlin {

class TargetGenerator : public SubComponent {
public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Merlin::TargetGenerator, int, int)

    TargetGenerator(ComponentId_t cid) :
        SubComponent(cid) {}

    ~TargetGenerator() {}

    virtual void initialize(int id, int num_peers) {}
    virtual int getNextValue(void) = 0;
    virtual void seed(uint32_t val) {}
};

} //namespace Merlin
} //namespace SST

#endif
