// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <sstream>      // std::stringstream


#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/interfaces/stringEvent.h>
#include <cstdlib>

#include "cache.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

Cache::CacheBlock* Cache::LRUCacheRow::getNext(){
	SimTime_t t = (SimTime_t)std::numeric_limits<unsigned long long>::max();

	int lru = -1;
	for ( int i = 0 ; i < cache->n_ways ; i++ ) {
		if ( blocks[i].isAssigned() )     continue; // Assigned, waiting for incoming
		if (  0 < blocks[i].locked )      continue; // Currently in use
		if ( NULL != blocks[i].loadInfo ) continue; // Assigned, but other work is being done

		if ( !blocks[i].isValid() ) {
			lru = i;
			break;
		}
		if ( blocks[i].last_touched <= t ) {
			t = blocks[i].last_touched;
			lru = i;
		}
	}
	if ( lru < 0 ) {
		cache->dbg.output(CALL_INFO, "No empty slot in this row.\n");
		return NULL;
	}

	cache->dbg.output(CALL_INFO, "Row LRU is block %p [0x%"PRIx64", [%d.%d]].\n", &blocks[lru], blocks[lru].baseAddr, blocks[lru].status, blocks[lru].user_locked);
	return &blocks[lru];
}

/************************ Cache::CacheRow ************************/

Cache::CacheBlock* Cache::CacheRow::getNext(void){
    _abort(CacheRow, "not callable");
    return NULL;
}


void Cache::CacheRow::addWaitingEvent(MemEvent *ev, SourceType_t src){
    Addr blockaddr = cache->addrToBlockAddr(ev->getAddr());
    waitingEvents[blockaddr].push_back(std::make_pair(ev, src));
    cache->dbg.output(CALL_INFO, "Event is number %zu in queue for this row.\n",
            waitingEvents[blockaddr].size());
    printRow();
}


void Cache::CacheRow::printRow(void){
    for ( int i = 0 ; i < cache->n_ways ; i++ ) {
        cache->dbg.output(CALL_INFO, "\t\tBlock [0x%"PRIx64" [%d.%d]] is: %s\n",
                blocks[i].baseAddr,
                blocks[i].status, blocks[i].user_locked,
                blocks[i].isAssigned() ?
                "Assigned" :
                (blocks[i].locked>0) ?
                "Locked!":
                "Open."
             );
    }
}

Cache::CacheBlock* Cache::MRUCacheRow::getNext(){

	SimTime_t t = (SimTime_t)-1;
	int mru = -1;
	for ( int i = 0 ; i < cache->n_ways ; i++ ) {
		if ( blocks[i].isAssigned() ) continue; // Assigned, waiting for incoming
		if (  0 < blocks[i].locked ) continue; // Currently in use
		if ( NULL != blocks[i].loadInfo ) continue; // Assigned, but other work is being done

		if ( !blocks[i].isValid() ) {
			mru = i;
			break;
		}
		if ( blocks[i].last_touched > t ) {
			t = blocks[i].last_touched;
			mru = i;
		}
	}
	if ( mru < 0 ) {
		cache->dbg.output(CALL_INFO, "No empty slot in this row.\n");
		return NULL;
	}

	cache->dbg.output(CALL_INFO, "Row MRU is block %p [0x%"PRIx64", [%d.%d]].\n", &blocks[mru], blocks[mru].baseAddr, blocks[mru].status, blocks[mru].user_locked);
	return &blocks[mru];

};

//Round Robin
Cache::CacheBlock* Cache::RRCacheRow::getNext(){

	int RR = -1;
	for ( int i = 0 ; i < cache->n_ways ; i++ ) {
		if ( blocks[i].isAssigned() )     continue; // Assigned, waiting for incoming
		if (  0 < blocks[i].locked )      continue; // Currently in use
		if ( NULL != blocks[i].loadInfo ) continue; // Assigned, but other work is being done

		if ( !blocks[i].isValid() ) {
			RR = i;
			tracker = (tracker + 1) % cache->n_ways;
			break;
		}
	}

	for ( int i = 0 ; i < cache->n_ways ; i++ ) {
		if ( !blocks[tracker].isAssigned() &&  0 >= blocks[tracker].locked &&  NULL == blocks[tracker].loadInfo ) RR = tracker;
		else tracker = (tracker + 1) % cache->n_ways;
	}
	if ( RR < 0 ) {
		cache->dbg.output(CALL_INFO, "No empty slot in this row.\n");
		return NULL;
	}

	cache->dbg.output(CALL_INFO, "Row RR is block %p [0x%"PRIx64", [%d.%d]].\n", &blocks[RR], blocks[RR].baseAddr, blocks[RR].status, blocks[RR].user_locked);
	return &blocks[RR];

};

Cache::CacheBlock* Cache::RandomCacheRow::getNext(){

	int random = -1;
	for ( int i = 0 ; i < cache->n_ways ; i++ ) {
		if ( blocks[i].isAssigned() )     continue; // Assigned, waiting for incoming
		if (  0 < blocks[i].locked )      continue; // Currently in use
		if ( NULL != blocks[i].loadInfo ) continue; // Assigned, but other work is being done

		if ( !blocks[i].isValid() ) {
			random = i;
			break;
		}
		random = rand() % blocks.size();
	}
	if ( random < 0 ) {
		cache->dbg.output(CALL_INFO, "No empty slot in this row.\n");
		return NULL;
	}

	cache->dbg.output(CALL_INFO, "Row Random is block %p [0x%"PRIx64", [%d.%d]].\n", &blocks[random], blocks[random].baseAddr, blocks[random].status, blocks[random].user_locked);
	return &blocks[random];

};


unsigned hash( const char *s ){
    unsigned hashVal;

    for( hashVal = 0 ; *s != '\0' ; s++ )
        hashVal = *s + 31 * hashVal;

    return  hashVal;
}

Cache::CacheBlock* Cache::SkewedAssociativeCacheRow::getNext(){

	SimTime_t t = (SimTime_t)std::numeric_limits<unsigned long long>::max();
	int lru = -1;
	for ( int i = 0 ; i < cache->n_ways ; i++ ) {
		if ( blocks[i].isAssigned() )     continue; // Assigned, waiting for incoming
		if ( 0 < blocks[i].locked )       continue; // Currently in use
		if ( NULL != blocks[i].loadInfo ) continue; // Assigned, but other work is being done

		if ( !blocks[i].isValid() ) {
			lru = i;
			break;
		}
		if ( blocks[i].last_touched < t ) {
			t = blocks[i].last_touched;
			lru = i;
		}
	}

	Cache::CacheBlock *result;

	if(lru == 0) result = &blocks[lru];
	else{
		std::stringstream ss;
		ss << (uint64_t)cache->last_row_visited << ' ' << lru;
		std::string key = ss.str();
		unsigned int hashValue = cache->hash(key.c_str());
		hashValue = hashValue % cache->n_rows;
		result = &cache->database[hashValue]->blocks[lru];
	}

	if ( lru < 0 ) {
		cache->dbg.output(CALL_INFO, "No empty slot in this row.\n");
		return NULL;
	}

	//cache->dbg.output(CALL_INFO, "Row LRU is block %p [0x%"PRIx64", [%d.%d]].\n", &blocks[lru], blocks[lru].baseAddr, blocks[lru].status, blocks[lru].user_locked);
	return result;

};
