/// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */



#ifndef _H_SST_TLBSTRUCTURE
#define _H_SST_TLBSTRUCTURE

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/componentExtension.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>

#include<string>

#include "utils.h"
#include "TLBentry.h"
#include "TLBUnit.h"
#include "PageTableWalker.h"

#include<map>
#include<vector>


// This is going to define the Samba/TLB Hierarchy for each core
// By default, we assume an LRU replacement policy


namespace SST { namespace SambaComponent{

	class TLBhierarchy : public ComponentExtension
	{

		Output * output;

        // ==== Params

		int coreID;  // This keeps track of which core the TLBhierarchy belongs to, 
                     //     we assume typical private TLBs as in current x86 processor
		int levels;  // number of TLB levels, 2 will make an L1 and L2
		int latency; // The access latency in ns


        //=== Params that only apply if emulate_faults is true
	    int emulate_faults; // whether page faults are emulated
        
		int page_placement; // Enable page migration if Opal is used
		int shootdown_delay;
		int page_swapping_delay;
	    int max_shootdown_width;
		uint64_t memory_size;
		uint32_t ptw_confined;


        // === Objects (links and child classes)
		SST::Link * to_cache;
		SST::Link * to_cpu;

		std::map<int, TLB *> TLB_CACHE; // TLB objects for each level: 1-indexed, (i.e. TLB_CACHE[1] is L1 TLB)
		PageTableWalker * PTW;          // page table walker of the TLB hierarchy


		SST::Cycle_t curr_time; // Holds the current time


        //==== Signals 
        //- these will be set to 1 or 0 to change our behavior
        //- pointers are passed to our children so they can set these
        
		int hold;       // tells TLB hierarchy to stall due to emulated page fault
		int shootdown;  // tells TLB hierarchy to invalidate all TLB entries due to TLB Shootdown from other cores

        // Referenced by pageTableWalker using pointer passing?
		int hasInvalidAddrs;

        //passed to pagetablewalker?? not used???
		uint64_t timeStamp;


        //======== Event buffers?

		std::vector<SST::MemHierarchy::MemEventBase *> mem_reqs; // holds the current requests to be translated

		std::vector<std::pair<Address_t, int> > invalid_addrs;  // holds the invalidation requests
		std::map<SST::MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> mem_reqs_sizes;
                                                        // holds the current requests to be translated
		std::map<SST::Event *, uint64_t> time_tracker;   // used to track time spent on translating each request
		
		// This represents the maximum number of outstanding requests for this structure
		//int max_outstanding; //TODO: TEMP JVOROBY 2021.11: i think this is unused? will try to rebuild without it


        //======= Page table stuff:

		// Holds CR3 value of current context (i.e. base of page table)
		Address_t *CR3;

		// Holds the PGD, PUD, PMT, PTE physical pointers
		std::map<Address_t, Address_t> * PGD; // key is 9 bits 39-47, i.e., VA/(4096*512*512*512)
		std::map<Address_t, Address_t> * PUD; // key is 9 bits 30-38, i.e., VA/(4096*512*512)
		std::map<Address_t, Address_t> * PMD; // key is 9 bits 21-29, i.e., VA/(4096*512)
		std::map<Address_t, Address_t> * PTE; // key is 9 bits 12-20, i.e., VA/(4096)         
                                              // PTE should give you the exact physical address of the page

		// The structures below are used to quickly check if the page is mapped or not
		std::map<Address_t,int> * MAPPED_PAGE_SIZE4KB;
		std::map<Address_t,int> * MAPPED_PAGE_SIZE2MB;
		std::map<Address_t,int> * MAPPED_PAGE_SIZE1GB;

		std::map<Address_t,int> *PENDING_PAGE_FAULTS;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PGD;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PUD;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PMD;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PTE;
		std::map<Address_t,int> *PENDING_SHOOTDOWN_EVENTS;


		public:


		void finish();

		// Handling Events arriving from Cache, mostly responses from memory hierarchy
		void handleEvent_CACHE(SST::Event * event);
		// Handling Events arriving from CPU, mostly requests from Ariel
		void handleEvent_CPU(SST::Event * event);


		void setPageTablePointers(  Address_t * cr3, 
                                    std::map<Address_t, Address_t> * pgd,  
                                    std::map<Address_t, Address_t> * pud,  
                                    std::map<Address_t, Address_t> * pmd, 
                                    std::map<Address_t, Address_t> * pte,
                                    std::map<Address_t,int> * gb,  
                                    std::map<Address_t,int> * mb,  
                                    std::map<Address_t,int> * kb, 
                                    std::map<Address_t,int> * pr, 
                                    int *cr3I, 
                                    std::map<Address_t,int> *pf_pgd,
                                    std::map<Address_t,int> *pf_pud,  
                                    std::map<Address_t,int> *pf_pmd, 
                                    std::map<Address_t,int> * pf_pte)
		{
                        CR3 = cr3;
                        PGD = pgd;
                        PUD = pud;
                        PMD = pmd;
                        PTE = pte;
                        MAPPED_PAGE_SIZE4KB = kb;
                        MAPPED_PAGE_SIZE2MB = mb;
                        MAPPED_PAGE_SIZE1GB = gb;
                        PENDING_PAGE_FAULTS = pr;
                        PENDING_PAGE_FAULTS_PGD = pf_pgd;
						PENDING_PAGE_FAULTS_PUD = pf_pud;
						PENDING_PAGE_FAULTS_PMD = pf_pmd;
						PENDING_PAGE_FAULTS_PTE = pf_pte;

			if(PTW!=nullptr)
				//PTW->setPageTablePointers(cr3, pgd, pud, pmd, pte, gb, mb, kb, pr, sr);
				PTW->setPageTablePointers(cr3, pgd, pud, pmd, pte, gb, mb, kb, pr, cr3I, pf_pgd, pf_pud, pf_pmd, pf_pte);

		}
		// Constructor for component
		TLBhierarchy(ComponentId_t id, Params& params);
		TLBhierarchy(ComponentId_t id, Params& params, int tlb_id);
		TLBhierarchy(SST::ComponentId_t id, int tlb_id);
		TLBhierarchy(SST::ComponentId_t id, int tlb_id, int level, Params& params);

		PageTableWalker * getPTW(){return PTW;}

		bool tick(SST::Cycle_t x);

		// Doing the translation
		Address_t translate(Address_t VA);

		Statistic<uint64_t>* total_waiting;
		//	std::map<int, Statistic<uint64_t>*> statTLBHits;

		//	std::map<int, Statistic<uint64_t>*> statTLBMisses;
		// Sets the to cache link, this should be called from Samba component when creating links between TLB structures and CPUs
		bool setCacheLink(SST::Link * TO_CACHE) { to_cache = TO_CACHE; return true;}

		// Sets the to cpu link, this should be called from Samba component when creating links between TLB structures and CPUs
		bool setCPULink(SST::Link * TO_CPU) { to_cpu = TO_CPU; return true;}

		// Setting the memory link of the page table walker
		bool setPTWLink(SST::Link * l) { PTW->set_ToMem(l); return true;}

		// Setting the line size for the page table walker's dummy requests
		void setLineSize(uint64_t size) { PTW->setLineSize(size); }


	};

}}

#endif
