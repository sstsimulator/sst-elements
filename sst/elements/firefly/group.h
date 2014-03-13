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
#include <virtNic.h>

namespace SST {
namespace Firefly {

class Group
{
  public:
    Group( VirtNic* ) : 
        m_rank( -1 ) 
    {}

    Group( VirtNic*, int size ) :
        m_rank( -1 ) 
    {
        m_rankMap.resize(size);
    }

    int getMyRank() {
        return m_rank;
    }

    size_t size() { return m_rankMap.size(); } 

    void set( int pos, int nid, int core ) {
        m_rankMap[pos] = m_virtNic->calc_virtId( nid, core );
    }

    void setMyRank( int rank) {
        m_rank = rank;
    }

    int getNodeId( int pos ) {
        return m_rankMap[pos];
    }

    int getCoreId( int pos ) {
        return m_virtNic->calc_vNic( m_rankMap[pos] );
    }

  private:
    VirtNic* m_virtNic;
    int m_rank;
    std::vector< int > m_rankMap;
};
}
}
#endif
