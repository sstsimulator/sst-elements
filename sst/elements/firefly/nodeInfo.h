// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_NODEINFO_H
#define COMPONENTS_FIREFLY_NONEINFO_H

#include <vector>

namespace SST {
namespace Firefly {

class NodeInfo
{
  public:
    NodeInfo( SST::Params& params) {
        m_coreNum = params.find_integer( "coreNum" );
        m_numCores = params.find_integer( "numCores" );
    }
    int coreNum() {
        return m_coreNum;
    }
    int numCores() {
        return m_numCores;
    }
  private:
    int m_coreNum;
    int m_numCores;
};
}
}
#endif
