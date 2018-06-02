// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_COMPONENTS_FIREFLY_HEAP_ADDRS
#define _H_COMPONENTS_FIREFLY_HEAP_ADDRS

#include <map>
#include <set>
#include <string>

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#define _H_HEAP_ADDRS_DBG 0 

namespace SST {
namespace Firefly {

class HeapAddrs {

	struct Entry {
		Entry() : free( false) {}
		Entry( uint64_t _addr, size_t _length, bool _free = true) : 
			addr(_addr), length(_length), free(_free) {}
		uint64_t addr;
		size_t length;
		bool	free;
		Entry*  prev;
		Entry*  next;
	};

	typedef std::map< uint64_t, Entry* > EntryMap;
	typedef std::set< Entry* > EntrySet;

  public:

	~HeapAddrs() {
		EntrySet::iterator iter = m_free.begin();
		for ( ; iter != m_free.end(); ++iter ) {
			delete *iter;
		}	
		EntryMap::iterator iter2 = m_used.begin();
		for ( ; iter2 != m_used.end(); ++iter2 ) {
			delete iter2->second;
		}	
		delete m_head;
		delete m_tail;
	}

	HeapAddrs( uint64_t start, size_t length ) {

#if _H_HEAP_ADDRS_DBG
		printf("%s() %#lx %lu\n",__func__,start,length);
#endif
		m_head = new Entry;
		m_tail = new Entry;

		m_head->next = m_tail;
		m_head->prev = NULL;
		m_tail->prev = m_head;
		m_tail->next = NULL;

		Entry* entry = new Entry( start, length );
		insertAfter( m_head, entry );
		m_free.insert( entry ); 

#if _H_HEAP_ADDRS_DBG
		print( m_head );
#endif
	}

	void insertAfter( Entry* prev, Entry* n ) {
		Entry* next = prev->next;
		prev->next = n;	
		next->prev = n;
		n->next = next;
		n->prev = prev;
	}

	void insertBefore( Entry* next, Entry* n ) {
		Entry* prev = next->prev;
		prev->next = n;	
		next->prev = n;
		n->next = next;
		n->prev = prev;
	}

	void erase( Entry* pos ) {
		Entry* prev = pos->prev;
		Entry* next = pos->next;
		prev->next = next;
		next->prev = prev;
	}

	uint64_t alloc( size_t length ) {

		length = (length + 16) & ~15;

#if _H_HEAP_ADDRS_DBG
		printf("%s() need %lu \n", __func__,length ); 
#endif

		EntrySet::iterator iter = m_free.begin();
		for ( ; iter != m_free.end(); ++iter ) {
			Entry* entry = (*iter);

#if _H_HEAP_ADDRS_DBG
			printf("check block %#lx %lu\n", (*iter)->addr, (*iter)->length ); 
#endif
			
			if ( length <= entry->length ) {
				Entry* tmp;
				
				if ( length < entry->length ) {

					tmp = new Entry( entry->addr, length, false );

					insertBefore( entry, tmp );	

					entry->length -= length;
					entry->addr += length;

				} else {
 					tmp = entry;
					entry->free = false;
					m_free.erase(tmp);
				}

				m_used[tmp->addr] = tmp;

#if _H_HEAP_ADDRS_DBG
				printSet( "free", m_free );
				printMap( "used", m_used );
				print( m_head );
				printf("%s() free=%lu used=%lu len=%lu\n",__func__,
								m_free.size(),m_used.size(), length);
#endif
				return tmp->addr;
			}
		} 
		printf("%s() free=%lu used=%lu len=%lu\n",__func__,m_free.size(),m_used.size(), length);
		assert(0);
	}
	void free( uint64_t addr ) {
		assert( m_used.find( addr ) != m_used.end() );
		
		Entry* entry = m_used[addr];	
		m_used.erase(addr);
		size_t length = entry->length;

#if _H_HEAP_ADDRS_DBG
		printf("%s() %#x %lu \n",__func__,addr,entry->length);
#endif

		Entry* prev = entry->prev;
		Entry* next = entry->next;

		if ( prev->free ) {
			prev->length += entry->length;

			if ( next->free ) {
				prev->length += next->length;
				m_free.erase( next );
				erase( next );
				delete next;
			}
			erase( entry );
			delete entry;
		} else if ( next->free ) {
			next->length += entry->length;
			next->addr = entry->addr;
			erase( entry );
			delete entry;
		} else {
			entry->free = true;
			m_free.insert( entry );
		}

#if _H_HEAP_ADDRS_DBG
		printSet( "free", m_free );
		printMap( "used", m_used );
		print( m_head );
		printf("%s() free=%lu used=%lu len=%lu\n",__func__,m_free.size(),m_used.size(), length);
#endif
	}

  private:

#if _H_HEAP_ADDRS_DBG
	void printMap( const std::string& name, EntryMap& list ) {
		printf("%s list:\n",name.c_str());
		EntryMap::iterator iter = list.begin();
		for ( ; iter != list.end(); ++iter ) {
			Entry* entry = iter->second;
			printf("\t%#lx %lu\n", iter->first, entry->length);
		}
	}
	void printSet( const std::string& name, EntrySet& list ) {
		printf("%s list:\n",name.c_str());
		EntrySet::iterator iter = list.begin();
		for ( ; iter != list.end(); ++iter ) {
			printf("\t%#lx %lu\n", (*iter)->addr, (*iter)->length);
		}
	}

	void print( Entry* cur ) {
		cur = cur->next;	
		printf("all:\n");
		while ( cur  && cur->next ) {
			printf("  addr=%#lx length=%lu %s\n",cur->addr, cur->length, cur->free ? "free" : "used" );
			cur = cur->next;
		}
		printf("\n");
	}
#endif

	EntrySet m_free;
	EntryMap m_used;
	Entry*   m_head;
	Entry*   m_tail;
};
}
}

#endif
