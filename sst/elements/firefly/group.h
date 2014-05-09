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
		return m_nidMap.rbegin()->first * m_virtNic->getNumCores(); 
	}

    void set( int rank, int nid, int num ) {
		m_nidMap[ rank ] = nid;		
		m_nidMap[ rank + num ] = -1;
    }

    void initMyRank() {
		int nodeId = m_virtNic->getRealNicId();
		std::map<int,int>::iterator iter;

		for ( iter = m_nidMap.begin(); iter != m_nidMap.end(); ++iter ) {
		
			std::map<int,int>::iterator next = iter;
			++next;
			int len = next->first - iter->first;
			if ( nodeId >= iter->second && nodeId < iter->second + len ) {
        		m_rank = iter->first + (nodeId - iter->second);
				m_rank *= m_virtNic->getNumCores();
				m_rank += m_virtNic->getCoreId();
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
		int numCores = m_virtNic->getNumCores();
		std::map<int,int>::iterator iter;
		
		for ( iter = m_nidMap.begin(); iter != m_nidMap.end(); ++iter ) {
			std::map<int,int>::iterator next = iter;
			++next;
			if ( rank/numCores >= iter->first && rank/numCores < next->first ) {
				nid = iter->second + (rank/numCores - iter->first); 
				break;
			}
		}
        return m_virtNic->calcVirtNicId( nid, rank % numCores );
    }

  private:
    VirtNic* m_virtNic;
    int 	m_rank;
    std::map< int, int> m_nidMap;

};
}
}
#endif
