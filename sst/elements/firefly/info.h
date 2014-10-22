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
    Info() : m_currentGroupID(0) {}
	~Info() {
    	std::map<Hermes::Communicator, Group*>::iterator iter; 
		for ( iter = m_groupMap.begin(); iter != m_groupMap.end(); ++iter ) {
			delete (*iter).second;	
		}
	}

    enum GroupType { Dense, Identity }; 
    Hermes::Communicator newGroup( Hermes::Communicator groupID, 
                GroupType type = Dense ) {

        assert( m_groupMap.find( groupID ) == m_groupMap.end() );
        switch( type) {
          case Dense:
            m_groupMap[groupID] = new DenseGroup();
            break;
          case Identity:
            m_groupMap[groupID] = new IdentityGroup();
            break;
        }
        return groupID;
    }

    Hermes::Communicator newGroup( GroupType type = Dense ) {
        return newGroup( genGroupID(), type );
    }

    Group* getGroup( Hermes::Communicator group ) {
		if ( m_groupMap.empty() ) return NULL;
        return m_groupMap[group];
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

    Hermes::Communicator genGroupID() {
        while ( m_groupMap.find(m_currentGroupID) != m_groupMap.end() ) {
            ++m_currentGroupID;
        }
        return m_currentGroupID;
    }

    std::map<Hermes::Communicator, Group*> m_groupMap;
    Hermes::Communicator    m_currentGroupID;
};

}
}
#endif
