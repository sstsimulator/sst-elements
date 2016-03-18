// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   sieveController.h
 */

#ifndef _SIEVECONTROLLER_H_
#define _SIEVECONTROLLER_H_

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>


#include "../cacheArray.h"
#include "../replacementManager.h"
#include "../util.h"
#include "../cacheListener.h"
#include "../../ariel/arielalloctrackev.h"


namespace SST { namespace MemHierarchy {

using namespace std;


class Sieve : public SST::Component {
public:

    typedef CacheArray::CacheLine CacheLine;
    typedef unsigned int uint;
    typedef uint64_t uint64;

    virtual void init(unsigned int);
    virtual void finish(void);
    
    /** Creates cache componennt */
    static Sieve* sieveFactory(SST::ComponentId_t id, SST::Params& params);
    
    /** Computes the 'Base Address' of the requests.  The base address point the first address of the cache line */
    Addr toBaseAddr(Addr addr){
        Addr baseAddr = (addr) & ~(cacheArray_->getLineSize() - 1);  //Remove the block offset bits
        return baseAddr;
    }
    
private:
    struct SieveConfig;
    typedef map<Addr, ArielComponent::arielAllocTrackEvent*> allocMap_t;
    typedef list<ArielComponent::arielAllocTrackEvent*> allocList_t;
    typedef pair<uint64_t, uint64_t> rwCount_t;
    typedef map<ArielComponent::arielAllocTrackEvent*, 
                rwCount_t > allocCountMap_t;
    /** Name of the output file */
    string outFileName;
    /** output file counter */
    uint64_t outCount;
    /** All Allocations */
    allocCountMap_t allocMap;
     /** All allocations in list form */
    allocList_t allocList;
    /** Active Allocations */
    allocMap_t actAllocMap; 
    /** misses not associated with an alloc'd region */
    uint64_t unassociatedMisses;

    void recordMiss(Addr addr, bool isRead);
    
    /** Constructor for Sieve Component */
    Sieve(ComponentId_t id, Params &params, CacheArray * cacheArray, Output * output);
    
    /** Destructor for Sieve Component */
    ~Sieve();

    /** Function to find and configure links */
    void configureLinks();

    /** Handler for incoming link events.  */
    void processEvent(SST::Event* event);
    /** Handler for incoming allocation events.  */
    void processAllocEvent(SST::Event* event);

    /** output and clear stats to file  */
    void outputStats(int marker);
    
    CacheArray*         cacheArray_;
    Output*             output_;
    CacheListener*      listener_;
    vector<SST::Link*>  cpuLinks_;
    uint32_t            cpuLinkCount_;
    SST::Link* alloc_link;

    /* Statistics */
    Statistic<uint64_t>* statReadHits;
    Statistic<uint64_t>* statReadMisses;
    Statistic<uint64_t>* statWriteHits;
    Statistic<uint64_t>* statWriteMisses;


};

/*  Implementation Details
 
    The Sieve class serves as the main cache controller.  It is in charge or handling incoming
    SST-based events (cacheEventProcessing.cc) and forwarding the requests to the other system's
    subcomponents:
        - Cache Array:  Class in charge keeping track of all the cache lines in the cache.  The 
        Cache Line inner class stores the data and state related to a particular cache line.
 
        - Replacement Manager:  Class handles work related to the replacement policy of the cache.  
        Similar to Cache Array, this class is OO-based so different replacement policies are simply
        subclasses of the main based abstrac class (ReplacementMgr), therefore they need to implement 
        certain functions in order for them to work properly. Implemented policies are: least-recently-used (lru), 
        least-frequently-used (lfu), most-recently-used (mru), random, and not-most-recently-used (nmru).
 
        - Hash:  Class implements common hashing functions.  These functions are used by the Cache Array
        class.  For instance, a typical set associative array uses a simple hash function, whereas
        skewed associate and ZCaches use more advanced hashing functions.
 
 
    Key notes:
 
        - Class member variables have a suffix "_".

*/

}}

#endif
