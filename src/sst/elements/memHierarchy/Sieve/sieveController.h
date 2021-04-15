// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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

#include <unordered_map>

#include "sst/elements/memHierarchy/lineTypes.h"
#include "sst/elements/memHierarchy/cacheArray.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/replacementManager.h"
#include "sst/elements/memHierarchy/util.h"
#include "alloctrackev.h"


namespace SST { namespace MemHierarchy {

using namespace std;


class Sieve : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(Sieve, "memHierarchy", "Sieve", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simple cache filtering component to model last-level caches", COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS(
            {"cache_size",              "(string) Cache size with units. Eg. 4KiB or 1MiB"},
            {"associativity",           "(uint) Associativity of the cache. In set associative mode, this is the number of ways."},
            {"cache_line_size",         "(uint) Size of a cache line (aka cache block) in bytes.", "64"},
            {"profiler",                "(string) Name of profiling subcomponent. Currently only configured to work with cassini.AddrHistogrammer. Add params using 'profiler.paramName'", ""},
            {"debug",                   "(uint) Print debug information. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
            {"debug_level",             "(uint) Debugging/verbosity level. Between 0 and 10", "0"},
            {"output_file",             "(string) Name of file to output malloc information to. Will have sequence number (and optional marker number) and .txt appended to it. E.g. sieveMallocRank-3.txt", "sieveMallocRank"},
            {"reset_stats_at_buoy",     "(bool) Whether to reset allocation hit/miss stats when a buoy is found (i.e., when a new output file is dumped). Any value other than 0 is true." "0"} )

    SST_ELI_DOCUMENT_PORTS(
            {"cpu_link_%(port)d", "Ports connected to the CPUs", {"memHierarchy.MemEventBase"}},
            {"alloc_link_%(port)d", "Ports connected to the CPUs' allocation/free notification ports", {"memHierarchy.AllocTrackEvent"}} )

    SST_ELI_DOCUMENT_STATISTICS(
            {"ReadHits",    "Number of read requests that hit in the sieve", "count", 1},
            {"ReadMisses",  "Number of read requests that missed in the sieve", "count", 1},
            {"WriteHits",   "Number of write requests that hit in the sieve", "count", 1},
            {"WriteMisses", "Number of write requests that missed in the sieve", "count", 1},
            {"UnassociatedReadMisses", "Number of read misses that did not match a malloc", "count", 1},
            {"UnassociatedWriteMisses", "Number of write misses that did not match a malloc", "count", 1} )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( {"profiler", "(string) Name of profiling subcomponent. Currently only configured to work with cassini.AddrHistogrammer.", "SST::MemHierarchy::CacheListener"} )


/* Begin class definition */
    typedef unsigned int uint;
    typedef uint64_t uint64;

    /** Constructor for Sieve Component */
    Sieve(ComponentId_t id, Params &params);

    virtual void init(unsigned int);
    virtual void finish(void);

    /** Computes the 'Base Address' of the requests.  The base address point the first address of the cache line */
    Addr toBaseAddr(Addr addr){
        Addr baseAddr = (addr) & ~(lineSize_ - 1);  //Remove the block offset bits
        return baseAddr;
    }

private:
    struct mallocEntry {
        uint64_t id;    // ID assigned by ariel
        uint64_t size;  // Number of bytes
    };


    typedef map<Addr, mallocEntry> allocMap_t;
    typedef pair<uint64_t, uint64_t> rwCount_t;
    typedef std::unordered_map<uint64_t, rwCount_t > allocCountMap_t;


    /** Name of the output file */
    string outFileName;
    /** output file counter */
    uint64_t outCount;
    /** All Allocations */
    allocCountMap_t allocMap;
     /** All allocations in list form */
    /** Active Allocations */
    allocMap_t activeAllocMap;
    /** misses not associated with an alloc'd region */

    void recordMiss(Addr addr, bool isRead);

    /** Destructor for Sieve Component */
    ~Sieve();

    /** Function to find and configure links */
    void configureLinks();

    /** Function to configure profiler, if any */
    void createProfiler(const Params &params);

    /** Handler for incoming link events.  */
    void processEvent(SST::Event* event, int link);
    /** Handler for incoming allocation events.  */
    void processAllocEvent(SST::Event* event);

    /** output and clear stats to file  */
    void outputStats(int marker);
    bool resetStatsOnOutput;

    CacheArray<SharedCacheLine>* cacheArray_;
    Output*             output_;
    vector<SST::Link*>  cpuLinks_;
    uint32_t            cpuLinkCount_;
    vector<SST::Link*>  allocLinks_;
    CacheListener*      listener_;
    uint64_t            lineSize_;

    /* Statistics */
    Statistic<uint64_t>* statReadHits;
    Statistic<uint64_t>* statReadMisses;
    Statistic<uint64_t>* statWriteHits;
    Statistic<uint64_t>* statWriteMisses;
    Statistic<uint64_t>* statUnassocReadMisses;
    Statistic<uint64_t>* statUnassocWriteMisses;

};

/*  Implementation Details

    The Sieve class serves as the main cache controller.  It is in charge of handling incoming
    memEvents and forwarding the requests to the other system's
    subcomponents:
        - Cache Array:  Class in charge keeping track of all the cache lines in the cache.  The
        Cache Line inner class stores the data and state related to a particular cache line.

        - Replacement Manager:  Class handles work related to the replacement policy of the cache.
        Similar to Cache Array, this class is OO-based so different replacement policies are simply
        subclasses of the main based abstract class (ReplacementMgr), therefore they need to implement
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
