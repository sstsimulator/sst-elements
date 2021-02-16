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


#ifndef COMPONENTS_MERLIN_ARBITRATION_SINGLE_ARB_H
#define COMPONENTS_MERLIN_ARBITRATION_SINGLE_ARB_H

#include <sst/core/module.h>

namespace SST {
namespace Merlin {

class SingleArbitration : public Module {

public:

    // Params are: num_vcs
    SST_ELI_REGISTER_MODULE_API(SST::Merlin::SingleArbitration,int16_t)

    SingleArbitration() : Module() {}
    virtual ~SingleArbitration() {}

    virtual int next() = 0;
    virtual void satisfied() = 0;
    // virtual void print() {}
};


}
}

#endif // COMPONENTS_MERLIN_ROUTER_H
