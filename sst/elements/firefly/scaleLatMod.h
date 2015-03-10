
// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_SCALELATMOD_H
#define COMPONENTS_FIREFLY_SCALELATMOD_H

#include "latencyMod.h"
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/unitAlgebra.h>


namespace SST {
namespace Firefly {

class ScaleLatMod : public LatencyMod { 

  public:

    ScaleLatMod( Params& params) {
    }
    ~ScaleLatMod(){};

    size_t getLatency( size_t value ) {
        return 0;
    }
};

}
}

#endif
