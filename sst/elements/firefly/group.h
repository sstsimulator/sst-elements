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

#ifndef COMPONENTS_FIREFLY_GROUP_H
#define COMPONENTS_FIREFLY_GROUP_H

#include <vector>
#include <virtNic.h>

namespace SST {
namespace Firefly {

class Group
{
  public:
    Group( VirtNic* nic ) : 
        m_virtNic( nic ),
        m_rank( -1 )
    {}

    int getMyRank() { return m_rank; }

    size_t size() { 
		return m_rankMap.rbegin()->first; 
	}

    void set( int rank, int nid, int num ) {
		m_rankMap[ rank ] = nid;		
		m_rankMap[ rank + num ] = -1;
    }

    void initMyRank() {
        int numCores = m_virtNic->getNumCores();
		int coreNum = m_virtNic->getCoreId();
		int nodeId = m_virtNic->getRealNicId();
		int nid = numCores * nodeId + coreNum;
		std::map<int,int>::iterator iter;

		for ( iter = m_rankMap.begin(); iter != m_rankMap.end(); ++iter ) {
		
			std::map<int,int>::iterator next = iter;
			++next;
			int len = next->first - iter->first;
			if ( nid >= iter->second && nid < iter->second + len ) {
        		m_rank = iter->first + (nid - iter->second);
				break;
			}
		}
		assert( -1 != m_rank );
    }

	// this is special case of rankToNid that handles wildcard
    int rankToNid( int rank ) {
        int nid = -1; 
        if ( -1 == rank ) {
            return -1;
        }
        nid = getNodeId( rank );
        return nid;
    } 

    int getNodeId( int rank ) {
		int nid = -1;
		std::map<int,int>::iterator iter;
		
		for ( iter = m_rankMap.begin(); iter != m_rankMap.end(); ++iter ) {
			std::map<int,int>::iterator next = iter;
			++next;
			if ( rank >= iter->first && rank < next->first ) {
				nid = iter->second + (rank - iter->first); 
				break;
			}
		}
        int numCores = m_virtNic->getNumCores();
        return m_virtNic->calcVirtNicId( nid / numCores, nid % numCores );
    }

  private:
    VirtNic* m_virtNic;
    int 	m_rank;
    std::map< int, int> m_rankMap;

};
}
}
#endif
