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

#ifndef COMPONENTS_FIREFLY_GROUP_H
#define COMPONENTS_FIREFLY_GROUP_H

#include <vector>

namespace SST {
namespace Firefly {

class Group {
  public:
    Group() {
    }

    Group( int size, int rank ) :
        m_rank( rank )
    {
        m_map.resize(size);
    }

    int rank() {
        return m_rank;
    }

    int size() {
        return m_map.size();
    }
    void mapRank( int rank, int foo ) {
        m_map[rank] = foo;
    }
 private:
    int m_rank;
    std::vector< int > m_map;
};
}
}
#endif
