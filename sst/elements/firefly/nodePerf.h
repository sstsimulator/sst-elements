// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_NODE_PERF_H
#define COMPONENTS_FIREFLY_NODE_PERF_H

#include "sst/elements/hermes/hermes.h"

namespace SST {
namespace Firefly {

class SimpleNodePerf : public NodePerf {

  public:
    SimpleNodePerf( Params& params ) {
        m_flops = params.find_floating("flops",0);
        m_bandwidth = params.find_floating("bandwidth",0);
    }

    virtual float getFlops() { return m_flops; }
    virtual float getBandwidth() { return m_bandwidth; }
    virtual float calcTimeNS_flops( int instructions ) { 
        return instructions / m_flops * 1000 * 1000 * 1000; 
    }
    virtual float calcTimeNS_bandwidth( int bytes ) { 
        return bytes / m_bandwidth * 1000 * 1000 * 1000;
    }

  private:
    float m_flops;
    float m_bandwidth;
};

}
}

#endif
