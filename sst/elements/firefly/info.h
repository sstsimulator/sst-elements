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

#ifndef COMPONENTS_FIREFLY_INFO_H
#define COMPONENTS_FIREFLY_INFO_H

#include "group.h"
#include "sst/elements/hermes/msgapi.h"

namespace SST {
namespace Firefly {

class Info {
  public:
	~Info() {
    	std::map<Hermes::Communicator, Group*>::iterator iter; 
		for ( iter = m_groupMap.begin(); iter != m_groupMap.end(); ++iter ) {
			delete (*iter).second;	
		}
	}
    void addGroup( Hermes::Communicator group, Group* x ) {
        m_groupMap[group] = x; 
    }

    Group* getGroup( Hermes::Communicator group ) {
		if ( m_groupMap.empty() ) return NULL;
        return m_groupMap[group];
    } 

    int rankToNodeId(Hermes::Communicator group, Hermes::RankID rank) {
        return m_groupMap[group]->getNodeId(rank);
    }

    int rankToWorldRank( Hermes::Communicator group, Hermes::RankID rank ) {
        assert( Hermes::GroupWorld == group ); 
        return rank;
    } 

    int worldRankToNid( int rank ) {
        return m_groupMap[Hermes::GroupWorld]->rankToNid( rank );
    }

    int nodeId() {
        return m_groupMap[Hermes::GroupWorld]->getNodeId( worldRank() );
    }

    int worldRank() {
        if ( m_groupMap.empty() ) {
            return -1;
        } else {
            return m_groupMap[Hermes::GroupWorld]->getMyRank();
        }
    }
    unsigned sizeofDataType( Hermes::PayloadDataType type ) {
        switch( type ) {
        case Hermes::CHAR:
            return sizeof( char );
        case Hermes::INT:
            return sizeof( int );
        case Hermes::LONG:
            return sizeof( long );
        case Hermes::DOUBLE:
            return sizeof( double);
        case Hermes::FLOAT:
            return sizeof( float );
        default:
            assert(0);
        }
    } 

  private: 
    std::map<Hermes::Communicator, Group*> m_groupMap;
};

}
}
#endif
