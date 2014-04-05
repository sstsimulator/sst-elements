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

//#define MEMORY_PIG
class Group
{
  public:
    Group( VirtNic* nic ) : 
        m_virtNic( nic ),
        m_rank( -1 ) 
    {}

    Group( VirtNic* nic, int size ) :
        m_virtNic( nic ),
        m_rank( -1 ) 
    {
#ifdef MEMORY_PIG 
        m_rankMap.resize(size);
#else
        m_size = size;
#endif
    }

    int getMyRank() {
        return m_rank;
    }

#ifdef MEMORY_PIG 
    size_t size() { return m_rankMap.size(); }
#else
    size_t size() { return m_size; }
#endif

    void set( int pos, int nid, int core ) {
#ifdef MEMORY_PIG 
        assert( (size_t) pos < m_rankMap.size() );
        m_rankMap[pos] = m_virtNic->calc_virtId( nid, core );
#endif

    }

    void setMyRank( int rank) {
        m_rank = rank;
    }

    int nidToRank( int nid ) {
        int rank = -1;
        int vNic = m_virtNic->calc_vNic( nid );
        int realId = m_virtNic->calc_realId( nid );
        int numCores = m_virtNic->getNumCores();
        rank = numCores * realId + vNic;
        return rank;
    }

    int rankToNid( int rank ) {
        int nid = -1; 
        if ( -1 == rank ) {
            return -1;
        }
        nid = getNodeId( rank );
        return nid;
    } 

    int getNodeId( int pos ) {
#ifdef MEMORY_PIG 

        assert( (size_t) pos < m_rankMap.size() );
        int tmp = m_rankMap[pos];
        return tmp;
#else
        int numCores = m_virtNic->getNumCores();
        int nid = pos / numCores;
        int core = pos % numCores;//m_virtNic->getCoreNum();
        return m_virtNic->calc_virtId( nid, core );
#endif
    }

    int getCoreId( int pos ) {
#ifdef MEMORY_PIG 
        assert( (size_t) pos < m_rankMap.size() );
        int tmp = m_virtNic->calc_vNic( m_rankMap[pos] );
        return tmp;
#else
        return pos % m_virtNic->getNumCores();
#endif
    }

  private:
    VirtNic* m_virtNic;
    int m_rank;
#ifdef MEMORY_PIG 
    std::vector< int > m_rankMap;
#else
    size_t m_size;
#endif

};
}
}
#endif
