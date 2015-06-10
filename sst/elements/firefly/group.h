// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
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

class MapBase {
  public:
    virtual ~MapBase() {}
    virtual int getSize() = 0;
    virtual void initMapping( int from, int to, int range ) = 0;
    virtual int getMapping( int from ) = 0;
};

class Group : public MapBase
{
  public:
    Group() 
      : m_myRank( -1 )
    {}
    int getMyRank() {  return m_myRank; }
    void setMyRank( int rank ) { m_myRank = rank; }

  private:
    int                 m_myRank;
};

class RandomGroup : public Group 
{
  public:
    RandomGroup()  {} 

    int getSize() { return m_map.size(); }

    void initMapping( int from, int to, int range ) {
		
        if ( m_map.size() < from + range ) {
            m_map.resize(from + range);	
        }	
        for ( int i=0; i < range; i++ ) {
            m_map[from+i] = to+i;
        }
    }

    int getMapping( int from ) { return m_map[from]; }
  private:
    std::vector<int> m_map;
};

class IdentityGroup : public Group 
{
  public:
    IdentityGroup() : m_size(0) {} 

    int getSize() { return m_size; }

    void initMapping( int from, int to, int range ) {
        assert( from == to ); 
        assert( 0 == m_size );
        m_size = range; 
    }

    int getMapping( int from ) { return from; }
  private:
    int m_size;
};

class DenseGroup : public Group 
{
  public:
    int getSize() { 
        if ( m_map.empty() ) {
            return 0;
        }
        return m_map.rbegin()->first; 
    }

    void initMapping( int from, int to, int range ) {
        std::map<int,int>::iterator iter;

        for ( iter = m_map.begin(); iter != m_map.end(); ++iter ) {
            std::map<int,int>::iterator next = iter;
            ++next;
            if( next == m_map.end() ) {
                break;
            }
            if ( from == next->first ) {
                assert( -1 == next->second );
                if ( to == iter->second + ( next->first - iter->first ) ) {
                    m_map.erase( next->first );
                    m_map[ from + range ] = -1; 
                    return;
                }   
            }
        }

		m_map[ from ] = to;		
		m_map[ from + range ] = -1;
    }

    int getMapping( int from ) {
		int to = -1;
		std::map<int,int>::iterator iter;

		for ( iter = m_map.begin(); iter != m_map.end(); ++iter ) {
			std::map<int,int>::iterator next = iter;
			++next;
			if ( from >= iter->first && from < next->first ) {
				to = iter->second + (from - iter->first); 
				break;
			}
		}
        return to;
    }

  private:
    std::map< int, int> m_map;
};

}
}
#endif
