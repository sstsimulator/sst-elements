
// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_LATENCYMOD_H
#define COMPONENTS_FIREFLY_LATENCYMOD_H

#include <sst/core/module.h>
#include <sst/core/elementinfo.h>

namespace SST {
namespace Firefly {

class LatencyMod : public SST::Module {
  public:
    virtual size_t getLatency( size_t value ) = 0;
};

}
}

#endif
