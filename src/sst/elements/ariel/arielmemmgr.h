// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
               
                /* Memory levels */
                memoryLevels = (uint32_t) params.find<uint32_t>("memorylevels", 1);
	        defaultLevel = (uint32_t) params.find<uint32_t>("defaultlevel", 0);
                output->verbose(CALL_INFO, 1, 0, "Configuring for %" PRIu32 " memory levels; default level is %" PRIu32 ".\n", memoryLevels, defaultLevel);
	    
                /* Get page map policy */
	        ArielPageMappingPolicy mapPolicy = ArielPageMappingPolicy::LINEAR;
                std::string mappingPolicy = params.find<std::string>("pagemappolicy", "linear");

                if (mappingPolicy == "LINEAR" || mappingPolicy == "linear") {
		    mapPolicy = ArielPageMappingPolicy::LINEAR;
	        } else if(  mappingPolicy == "RANDOMIZED" || mappingPolicy == "randomized" ) {
		    mapPolicy = ArielPageMappingPolicy::RANDOMIZED;
	        } else {
		    output->fatal(CALL_INFO, -8, "Ariel memory manager - unknown page mapping policy \"%s\"\n", mappingPolicy.c_str());
	        }

                // Buffer to hold parameter search string
                char * level_buffer = (char*) malloc(sizeof(char) * 256);

                /* Configure page sizes, counts, etc. for each level */
		freePages = (std::deque<uint64_t>**) malloc(sizeof(std::deque<uint64_t>*) * memoryLevels);
	        pageSizes = (uint64_t*) malloc(sizeof(uint64_t) * memoryLevels);
	        
                uint64_t nextMemoryAddress = 0;
                for(uint32_t i = 0; i < memoryLevels; ++i) {
                    // Page size
                    sprintf(level_buffer, "pagesize%" PRIu32 , i);
                    pageSizes[i] = (uint64_t) params.find<uint64_t>(level_buffer, 4096);
		    output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " page size is %" PRIu64 "\n", i, pageSizes[i]);
	            
                    // Page count
                    sprintf(level_buffer, "pagecount%" PRIu32, i);
                    uint64_t pageCount = (uint64_t) params.find<uint64_t>(level_buffer, 131072);
                    
                    // Configure page pool
                    freePages[i] = new std::deque<uint64_t>();

                    if (ArielPageMappingPolicy::LINEAR == mapPolicy ){
                        output->verbose(CALL_INFO, 2, 0, "Page mapping policy is LINEAR map...\n");
                        output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " page count is %" PRIu64 "\n", i, pageCount);
                        for (uint64_t j = 0; j < pageCount; ++j) {
                            freePages[i]->push_back(nextMemoryAddress);
                            nextMemoryAddress += pageSizes[i];
                        }
                        output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " usable (free) page queue contains %" PRIu32 " entries\n", i,
                            (uint32_t) freePages[i]->size());
                    } else { // ArielPageMappingPolicy::RANDOMIZED
                        output->verbose(CALL_INFO, 2, 0, "Page mapping policy in RANDOMIZED map...\n");
                        output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " page count is %" PRIu64 "\n", i, pageCount);
                        
                        /* Randomize page ordering */
                        std::vector<uint64_t> preRandomizedPages;
                        preRandomizedPages.resize(pageCount);
                        MarsagliaRNG pageRandomizer(11, 201010101);
                                    
                        for(uint64_t j = 0; j < pageCount; ++j) {
                            preRandomizedPages[j] = nextMemoryAddress;
                            nextMemoryAddress += pageSizes[i];
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
                            output->verbose(CALL_INFO, 64, 0, "Page[%" PRIu64 ", Level: %" PRIu32 "] Physical Start=%" PRIu64 "\n",
                                    j, i, preRandomizedPages[j]);
                            freePages[i]->push_back(preRandomizedPages[j]);
                        }
                                       
                        output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " usable (free) page queue contains %" PRIu32 " entries\n", i,
                                (uint32_t) freePages[i]->size());

                    }
                }

                // Remaining data structures
                pageAllocations = (std::unordered_map<uint64_t, uint64_t>**) malloc(sizeof(std::unordered_map<uint64_t, uint64_t>*) * memoryLevels);
                for (uint32_t i = 0; i < memoryLevels; ++i) {
                    pageAllocations[i] = new std::unordered_map<uint64_t, uint64_t>();
                }
      
                pageTables = (std::unordered_map<uint64_t, uint64_t>**) malloc(sizeof(std::unordered_map<uint64_t, uint64_t>*) * memoryLevels);
                for (uint32_t i = 0; i < memoryLevels; ++i) {
                    pageTables[i] = new std::unordered_map<uint64_t, uint64_t>();
                }
                translationCache = new std::unordered_map<uint64_t, uint64_t>();
                translationCacheEntries = (uint32_t) params.find<uint32_t>("translatecacheentries", 4096);

                // Finally, prepopulate any page tables as needed 

	        for (uint32_t i = 0; i < memoryLevels; ++i) {
		    sprintf(level_buffer, "page_populate_%" PRIu32, i);
		    std::string popFilePath = params.find<std::string>(level_buffer, "");

		    if (popFilePath != "") {
			output->verbose(CALL_INFO, 1, 0, "Populating page tables for level %" PRIu32 " from %s...\n",
				i, popFilePath.c_str());
                        
                        // We need to load each entry and the create it in the page table for this level
                        FILE * popFile = fopen(popFilePath.c_str(), "rt");
                        uint64_t pinAddr = 0;

                        const uint64_t levelPageSize = pageSizes[i];
                        
                        while( ! feof(popFile) ) {
                            if (EOF == fscanf(popFile, "%" PRIu64 "\n", &pinAddr)) {
                                break;
                            }
                                
                            if (freePages[i]->size() == 0) {
                                output->fatal(CALL_INFO, -1, "Attempted to pin address %" PRIu64 " in level %" PRIu32 " but no free pages.\n",
                                    pinAddr, i);
                           }
                                
                            if (pinAddr % pageSizes[i] > 0) {
                                output->fatal(CALL_INFO, -1, "Attempted to pin address %" PRIu64 " in level %" PRIu32 " but address is not page aligned to page size %" PRIu64 "\n",
                                        pinAddr, i, pageSizes[i]);
                            }

                            const uint64_t freePhysical = freePages[i]->front();
                            freePages[i]->pop_front();
                                
                            output->verbose(CALL_INFO, 4, 0, "Pinning address %" PRIu64 " in level %" PRIu32 " (physical=%" PRIu64 "\n",
                                    pinAddr, i, freePhysical);
                                
                            pageTables[i]->insert( std::pair<uint64_t, uint64_t>( pinAddr, freePhysical ) );
                        }
                        
                        fclose(popFile);
                    }
	        }
	        
                free(level_buffer);


                /* Statistics used by all memory managers; managers may also have their own */
            } // End constructor
	    
            ~ArielMemoryManager() {};
	    
            /** Set default memory pool for allocation */
            virtual void setDefaultPool(uint32_t pool) {
	        defaultLevel = pool;
            }

            virtual uint32_t getDefaultPool() {
                return defaultLevel;
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

		uint32_t defaultLevel;
		uint32_t memoryLevels;
		uint64_t* pageSizes;
		std::deque<uint64_t>** freePages;
		std::unordered_map<uint64_t, uint64_t>** pageAllocations;
		std::unordered_map<uint64_t, uint64_t>** pageTables;
		std::unordered_map<uint64_t, uint64_t>* translationCache;
		uint32_t translationCacheEntries;
		bool translationEnabled;
};

}
}

#endif
