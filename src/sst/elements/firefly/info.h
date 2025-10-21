// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_INFO_H
#define COMPONENTS_FIREFLY_INFO_H

#include "group.h"
#include "sst/elements/hermes/msgapi.h"

#include <cassert>
#include <map>
#include <cstdint>

using namespace Hermes;

namespace SST {
namespace Firefly {

class Info {
  public:
    Info() : m_currentGroupID(MP::GroupWorld+1) {}
	~Info() {
    	std::map<MP::Communicator, Group*>::iterator iter;
		for ( iter = m_groupMap.begin(); iter != m_groupMap.end(); ++iter ) {
			delete (*iter).second;
		}
	}

    enum GroupType { Dense, Identity, Random, NetMap };
    MP::Communicator newGroup( MP::Communicator groupID,
                GroupType type = Dense ) {

        assert( m_groupMap.find( groupID ) == m_groupMap.end() );
        switch( type) {
          case Dense:
            m_groupMap[groupID] = new DenseGroup();
            break;
          case Identity:
            m_groupMap[groupID] = new IdentityGroup();
            break;
          case Random:
            m_groupMap[groupID] = new RandomGroup();
            break;
          case NetMap:
            m_groupMap[groupID] = new NetMapGroup();
            break;
        }
        return groupID;
    }

    MP::Communicator newGroup( GroupType type = Dense ) {
        return newGroup( genGroupID(), type );
    }
    void delGroup( MP::Communicator group ) {
        delete m_groupMap[ group ];
        m_groupMap.erase( group );
    }

    Group* getGroup( MP::Communicator group ) {
		if ( m_groupMap.empty() ) return NULL;
        return m_groupMap[group];
    }

    int worldRank() {
        if ( m_groupMap.empty() ) {
            return -1;
        } else {
            return m_groupMap[MP::GroupWorld]->getMyRank();
        }
    }

    unsigned sizeofDataType( MP::PayloadDataType type ) {
        switch( type ) {
        case MP::CHAR:
            return sizeof( char );
        case MP::SIGNED_CHAR:
            return sizeof( signed char );
        case MP::UNSIGNED_CHAR:
            return sizeof( unsigned char );
        case MP::INT:
            return sizeof( int );
        case MP::LONG:
            return sizeof( long );
        case MP::LONG_LONG:
            return sizeof( long long );
        case MP::UNSIGNED_INT:
            return sizeof( unsigned int );
        case MP::UNSIGNED_LONG:
            return sizeof( unsigned long );
        case MP::UNSIGNED_LONG_LONG:
            return sizeof( unsigned long long );
        case MP::INT8_T:
            return sizeof( std::int8_t );
        case MP::INT16_T:
            return sizeof( std::int16_t );
        case MP::INT32_T:
            return sizeof( std::int32_t );
        case MP::INT64_T:
            return sizeof( std::int64_t );
        case MP::UINT8_T:
            return sizeof( std::uint8_t );
        case MP::UINT16_T:
            return sizeof( std::uint16_t );
        case MP::UINT32_T:
            return sizeof( std::uint32_t );
        case MP::UINT64_T:
            return sizeof( std::uint64_t );
        case MP::DOUBLE:
            return sizeof( double);
        case MP::FLOAT:
            return sizeof( float );
        case MP::COMPLEX:
            return sizeof( double ) * 2;
        default:
            assert(0);
        }
    }

  private:

    MP::Communicator genGroupID() {
        while ( m_groupMap.find(m_currentGroupID) != m_groupMap.end() ) {
            ++m_currentGroupID;
        }
        return m_currentGroupID;
    }

    std::map<MP::Communicator, Group*> m_groupMap;
    MP::Communicator    m_currentGroupID;
};

}
}
#endif
