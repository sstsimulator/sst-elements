// Copyright 2013-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
        m_flops = params.find<double>("flops",0);
        m_bandwidth = params.find<double>("bandwidth",0);
    }

    virtual double getFlops() { return m_flops; }
    virtual double getBandwidth() { return m_bandwidth; }
    virtual double calcTimeNS_flops( int instructions ) { 
        return instructions / m_flops * 1000 * 1000 * 1000; 
    }
    virtual double calcTimeNS_bandwidth( int bytes ) { 
        return bytes / m_bandwidth * 1000 * 1000 * 1000;
    }

  private:
    double m_flops;
    double m_bandwidth;
};

}
}

#endif
