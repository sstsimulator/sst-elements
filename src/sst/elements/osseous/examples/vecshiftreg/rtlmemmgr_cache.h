// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_RTL_MEM_MANAGER_CACHE
#define _H_RTL_MEM_MANAGER_CACHE

#include <sst/core/output.h>
#include <sst/core/rng/marsaglia.h>

#include <stdint.h>
//#include <deque>
#include <vector>
#include <inttypes.h>
//#include <unordered_map>

#include "rtlmemmgr.h"

using namespace SST;
using namespace SST::RNG;

namespace SST {

namespace RtlComponent {

/* Base class for memory managers that cache translation addresses */
class RtlMemoryManagerCache : public RtlMemoryManager{

    public:

        /* ELI defines for RtlMemoryManagerCache subcomponents */
    #define RTL_ELI_MEMMGR_CACHE_STATS { "tlb_hits", "Hits in the simple Rtl TLB", "hits", 2 },\
        { "tlb_evicts",           "Number of evictions in the simple Rtl TLB", "evictions", 2 },\
        { "tlb_translate_queries","Number of TLB translations performed", "translations", 2 },\
        { "tlb_shootdown",        "Number of TLB clears because of page-frees", "shootdowns", 2 },\
        { "tlb_page_allocs",      "Number of pages allocated by the memory manager", "pages", 2 }

        /* Constructor
            *  Supports multiple memory pools with independent page sizes/counts
            *  Constructs free page sets for each memory pool
            *  Initializes a translation cache
            */
        RtlMemoryManagerCache(ComponentId_t id, Params& params) : RtlMemoryManager(id, params) {
            translationEnabled = params.find<bool>("vtop_translate", true);

            /* Common statistics */
            statTranslationCacheHits    = registerStatistic<uint64_t>("tlb_hits");
            statTranslationCacheEvict   = registerStatistic<uint64_t>("tlb_evicts");
            statTranslationQueries      = registerStatistic<uint64_t>("tlb_translate_queries");
            statTranslationShootdown    = registerStatistic<uint64_t>("tlb_shootdown");
            statPageAllocationCount     = registerStatistic<uint64_t>("tlb_page_allocs");

            /* Statistics used by all memory managers; managers may also have their own */
        } // End constructor

        ~RtlMemoryManagerCache() {};

        void AssignRtlMemoryManagerCache(std::unordered_map<uint64_t, uint64_t> Cache, uint32_t CacheEntries, bool Enabled) {
            
            // Set up translation cache
            translationCache = Cache;
            translationCacheEntries = CacheEntries;
            translationEnabled = Enabled;
            /*fprintf(stderr, "\ntranslationCache size in RtlMemorymanager is: %" PRIu64, translationCache.size());
            fprintf(stderr, "\ntranslationCacheEntries in RtlMemorymanager is: %" PRIu32, translationCacheEntries);
            fprintf(stderr, "\ntranslationEnabled in RtlMemorymanager is: %d", translationEnabled);*/

            return;
        }

    protected:
        Statistic<uint64_t>* statTranslationCacheHits;
        Statistic<uint64_t>* statTranslationCacheEvict;
        Statistic<uint64_t>* statTranslationQueries;
        Statistic<uint64_t>* statTranslationShootdown;
        Statistic<uint64_t>* statPageAllocationCount;

        std::unordered_map<uint64_t, uint64_t> translationCache;
        uint32_t translationCacheEntries;
        bool translationEnabled;

        void cacheTranslation(uint64_t virtualA, uint64_t physicalA) {
            // Remove the oldest entry if we do not have enough slots TODO is begin() really the oldest...?
            if(translationCache.size() == translationCacheEntries) {
            statTranslationCacheEvict->addData(1);
            translationCache.erase(translationCache.begin());
            }

            // Insert the translated entry into the cache
            translationCache.insert(std::pair<uint64_t, uint64_t>(virtualA, physicalA));
        }

};

}
}

#endif
