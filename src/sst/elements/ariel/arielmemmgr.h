// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_ARIEL_MEM_MANAGER
#define _H_ARIEL_MEM_MANAGER

#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/output.h>
#include <sst/core/rng/marsaglia.h>

#include <stdint.h>
#include <deque>
#include <vector>
#include <unordered_map>

using namespace SST;
using namespace SST::RNG;

namespace SST {

namespace ArielComponent {

enum ArielPageMappingPolicy {
    LINEAR,
    RANDOMIZED
};

class ArielMemoryManager : public SubComponent {

    public:
        /* ELI defines for ArielMemoryManager subcomponents */
    #define ARIEL_ELI_MEMMGR_PARAMS {"verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0"},\
        {"vtop_translate",  "Set to yes to perform virt-phys translation (TLB) or no to disable", "yes"},\
        {"pagemappolicy",   "Select the page mapping policy for Ariel [LINEAR|RANDOMIZED]", "LINEAR"},\
        {"translatecacheentries", "Keep a translation cache of this many entries to improve emulated core performance", "4096"}

    #define ARIEL_ELI_MEMMGR_STATS { "tlb_hits", "Hits in the simple Ariel TLB", "hits", 2 },\
        { "tlb_evicts",           "Number of evictions in the simple Ariel TLB", "evictions", 2 },\
        { "tlb_translate_queries","Number of TLB translations performed", "translations", 2 },\
        { "tlb_shootdown",        "Number of TLB clears because of page-frees", "shootdowns", 2 },\
        { "tlb_page_allocs",      "Number of pages allocated by the memory manager", "pages", 2 }

        /* Constructor
            *  Supports multiple memory pools with independent page sizes/counts
            *  Constructs free page sets for each memory pool
            *  Initializes a translation cache
            */
        ArielMemoryManager(Component * ownMe, Params& params) : SubComponent(ownMe) {
            owner = ownMe;
            int verbosity = params.find<int>("verbose", 0);
            output = new SST::Output("ArielMemoryManager[@f:@l:@p] ",
                verbosity, 0, SST::Output::STDOUT);

            translationEnabled = params.find<bool>("vtop_translate", true);

            /* Common statistics */
            statTranslationCacheHits    = registerStatistic<uint64_t>("tlb_hits");
            statTranslationCacheEvict   = registerStatistic<uint64_t>("tlb_evicts");
            statTranslationQueries      = registerStatistic<uint64_t>("tlb_translate_queries");
            statTranslationShootdown    = registerStatistic<uint64_t>("tlb_shootdown");
            statPageAllocationCount     = registerStatistic<uint64_t>("tlb_page_allocs");

            /* Get page map policy */
            mapPolicy = ArielPageMappingPolicy::LINEAR;
            std::string mappingPolicy = params.find<std::string>("pagemappolicy", "linear");

            if (mappingPolicy == "LINEAR" || mappingPolicy == "linear") {
            mapPolicy = ArielPageMappingPolicy::LINEAR;
            } else if(  mappingPolicy == "RANDOMIZED" || mappingPolicy == "randomized" ) {
            mapPolicy = ArielPageMappingPolicy::RANDOMIZED;
            } else {
            output->fatal(CALL_INFO, -8, "Ariel memory manager - unknown page mapping policy \"%s\"\n", mappingPolicy.c_str());
            }

            // Set up translation cache
            translationCache = new std::unordered_map<uint64_t, uint64_t>();
            translationCacheEntries = (uint32_t) params.find<uint32_t>("translatecacheentries", 4096);

            /* Statistics used by all memory managers; managers may also have their own */
        } // End constructor

        ~ArielMemoryManager() {};

        /** Set default memory pool for allocation */
        virtual void setDefaultPool(uint32_t pool) {
        }

        virtual uint32_t getDefaultPool() {
            return 0;
        }

        /** Request to translate an address, allocates if not found. */
        virtual uint64_t translateAddress(uint64_t virtAddr) = 0;

        /** Request to allocate a malloc, not supported by all memory managers */
        virtual bool allocateMalloc(const uint64_t size, const uint32_t level, const uint64_t virtualAddress) {
            output->verbose(CALL_INFO, 4, 0, "The instantiated ArielMemoryManager does not support malloc handling.\n");
            return false;
        }

        /** Request to free a malloc, not supported by all memory managers */
        virtual void freeMalloc(const uint64_t vAddr) {
            output->verbose(CALL_INFO, 4, 0, "The instantiated ArielMemoryManager does not support malloc handling.\n");
        };

        /** Print statistics: TODO move statistics to Statistics API */
        virtual void printStats() {};


    protected:
        Output* output;
        SST::Component* owner;

        Statistic<uint64_t>* statTranslationCacheHits;
        Statistic<uint64_t>* statTranslationCacheEvict;
        Statistic<uint64_t>* statTranslationQueries;
        Statistic<uint64_t>* statTranslationShootdown;
        Statistic<uint64_t>* statPageAllocationCount;

        std::unordered_map<uint64_t, uint64_t>* translationCache;
        uint32_t translationCacheEntries;
        bool translationEnabled;
        ArielPageMappingPolicy mapPolicy;

        void mapPagesLinear(uint64_t pageCount, uint64_t pageSize, uint64_t startAddr, std::deque<uint64_t>* freePagePool) {
            output->verbose(CALL_INFO, 2, 0, "Page mapping policy is LINEAR map...\n");
            uint64_t nextMemoryAddress = startAddr;

            for (uint64_t j = 0; j < pageCount; ++j) {
                freePagePool->push_back(nextMemoryAddress);
                nextMemoryAddress += pageSize;
            }
        }

        void mapPagesRandom(uint64_t pageCount, uint64_t pageSize, uint64_t startAddr, std::deque<uint64_t>* freePagePool) {
            output->verbose(CALL_INFO, 2, 0, "Page mapping policy is RANDOMIZED map...\n");

            uint64_t nextMemoryAddress = startAddr;

            /* Randomize page ordering */
            std::vector<uint64_t> preRandomizedPages;
            preRandomizedPages.resize(pageCount);
            MarsagliaRNG pageRandomizer(11, 201010101);

            for(uint64_t j = 0; j < pageCount; ++j) {
                preRandomizedPages[j] = nextMemoryAddress;
                nextMemoryAddress += pageSize;
            }

            for (uint64_t j = 0; j < (pageCount * 2); ++j) {
                const uint64_t swapLeft  = (pageRandomizer.generateNextUInt64() % pageCount);
                const uint64_t swapRight = (pageRandomizer.generateNextUInt64() % pageCount);
                if ( swapLeft != swapRight ) {
                    const uint64_t tmp = preRandomizedPages[swapRight];
                    preRandomizedPages[swapRight] = preRandomizedPages[swapLeft];
                    preRandomizedPages[swapLeft] = tmp;
                }
            }

            for (uint64_t j = 0; j < pageCount; ++j) {
                output->verbose(CALL_INFO, 64, 0, "Page[%" PRIu64 "] Physical Start=%" PRIu64 "\n",
                        j, preRandomizedPages[j]);
                freePagePool->push_back(preRandomizedPages[j]);
            }
        }

        void populatePageTable(std::string popFilePath, std::unordered_map<uint64_t, uint64_t> * pageTable, std::deque<uint64_t>* freePagePool, uint64_t pageSize) {
            FILE * popFile = fopen(popFilePath.c_str(), "rt");
            uint64_t pinAddr = 0;

            while( ! feof(popFile) ) {
                if (EOF == fscanf(popFile, "%" PRIu64 "\n", &pinAddr)) {
                    break;
                }

                if (freePagePool->size() == 0) {
                    output->fatal(CALL_INFO, -1, "Attempted to pin address %" PRIu64 " but no free pages.\n", pinAddr);
                }

                if (pinAddr % pageSize > 0) {
                    output->fatal(CALL_INFO, -1, "Attempted to pin address %" PRIu64 " but address is not page aligned to page size %" PRIu64 "\n",
                            pinAddr, pageSize);
                }

                const uint64_t freePhysical = freePagePool->front();
                freePagePool->pop_front();

                output->verbose(CALL_INFO, 4, 0, "Pinning address %" PRIu64 " (physical=%" PRIu64 "\n",
                            pinAddr, freePhysical);

                pageTable->insert( std::pair<uint64_t, uint64_t>( pinAddr, freePhysical ) );
            }

            fclose(popFile);
        }

        void cacheTranslation(uint64_t virtualA, uint64_t physicalA) {
            // Remove the oldest entry if we do not have enough slots TODO is begin() really the oldest...?
            if(translationCache->size() == translationCacheEntries) {
            statTranslationCacheEvict->addData(1);
            translationCache->erase(translationCache->begin());
            }

            // Insert the translated entry into the cache
            translationCache->insert(std::pair<uint64_t, uint64_t>(virtualA, physicalA));
        }

};

}
}

#endif
