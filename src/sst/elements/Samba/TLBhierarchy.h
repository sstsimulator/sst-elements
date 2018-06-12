/// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>

#include<string>

#include "TLBentry.h"
#include "TLBUnit.h"
#include "PageTableWalker.h"

#include<map>
#include<vector>


// This is going to define the Samba/TLB Hierarchy for each core
// By default, we assume an LRU replacement policy


namespace SST { namespace SambaComponent{

	class TLBhierarchy 
	{

		// This keeps track of which core the TLB belongs to, here we assume typical private TLBs as in current x86 processor
		int coreID;

		// Which level, this can be any value starting from 0 for L1 and up to N, in an N+1 levels TLB system
		int levels;

		Output * output;

		// This is the link to propogate requests to the cache hierarchy
		SST::Component * Owner;

		// If faults are emulated
	    int emulate_faults;

	    // Enable page migration if Opal is used
		int page_migration;

		// Page migration policy
		PageMigrationType page_migration_policy;

		uint64_t memory_size;

	    int max_shootdown_width;

		SST::Link * to_cache;


		SST::Link * to_cpu;

		SST::Link * to_opal;

		std::map<int, TLB *> TLB_CACHE;

		// Here is the defintion of the page table walker of the TLB hierarchy
		PageTableWalker * PTW;


		std::string clock_frequency_str;

		// Holds the current time 
		SST::Cycle_t curr_time;

		// This vector holds the current requests to be translated
		std::vector<SST::Event *> mem_reqs;

		// This tells TLB hierarchy to stall due to emulated page fault
		int hold;

		// This tells TLB hierarchy to invalidate all TLB entries due to TLB Shootdown from other cores
		int shootdown;

		// This tells TLB hierarchy to invalidate all TLB entries due to TLB Shootdown from the same core
		int own_shootdown;

		// This vector holds the invalidation requests
		std::list<Address_t> invalid_addrs;

		// This vector holds the current requests to be translated
		std::map<SST::Event *, long long int> mem_reqs_sizes;


		// This mapping is used to track the time spent of translating each request
		std::map<SST::Event *, uint64_t> time_tracker;
		// The access latency in ns
		int latency; 

		// This represents the maximum number of outstanding requests for this structure
		int max_outstanding; 

		// Holds CR3 value of current context
		Address_t *CR3;
		//
		// Holds the PGD physical pointers, the key is the 9 bits 39-47, i.e., VA/(4096*512*512*512)
		std::map<Address_t, Address_t> * PGD;

		// Holds the PUD physical pointers, the key is the 9 bits 30-38, i.e., VA/(4096*512*512)
		std::map<Address_t, Address_t> * PUD;

		// Holds the PMD physical pointers, the key is the 9 bits 21-29, i.e., VA/(4096*512)
		std::map<Address_t, Address_t> * PMD;

		// Holds the PTE physical pointers, the key is the 9 bits 12-20, i.e., VA/(4096)
		std::map<Address_t, Address_t> * PTE; // This should give you the exact physical address of the page


		// The structures below are used to quickly check if the page is mapped or not
		std::map<Address_t,int> * MAPPED_PAGE_SIZE4KB;
		std::map<Address_t,int> * MAPPED_PAGE_SIZE2MB;
		std::map<Address_t,int> * MAPPED_PAGE_SIZE1GB;

		std::map<Address_t,int> *PENDING_PAGE_FAULTS;
		std::map<Address_t,int> *PENDING_SHOOTDOWN_EVENTS;


		public:


		void finish();

		// Handling Events arriving from Cache, mostly responses from memory hierarchy
		void handleEvent_CACHE(SST::Event * event);
		// Handling Events arriving from CPU, mostly requests from Ariel
		void handleEvent_CPU(SST::Event * event);
		// Handling Events arriving from Opal, the memory manager
		void handleEvent_OPAL(SST::Event * event);


		void setPageTablePointers( Address_t * cr3, std::map<Address_t, Address_t> * pgd,  std::map<Address_t, Address_t> * pud,  std::map<Address_t, Address_t> * pmd, std::map<Address_t, Address_t> * pte,
				std::map<Address_t,int> * gb,  std::map<Address_t,int> * mb,  std::map<Address_t,int> * kb, std::map<Address_t,int> * pr, std::map<Address_t,int> * sr)
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
                        PENDING_SHOOTDOWN_EVENTS = sr;

			if(PTW!=NULL)
				PTW->setPageTablePointers(cr3, pgd, pud, pmd, pte, gb, mb, kb, pr, sr);

		}
		// Constructor for component
		TLBhierarchy(ComponentId_t id, Params& params);
		TLBhierarchy(ComponentId_t id, Params& params, int tlb_id);
		TLBhierarchy(int tlb_id,SST::Component * owner);
		TLBhierarchy(int tlb_id, int level, SST::Component * owner, Params& params);

		PageTableWalker * getPTW(){return PTW;}

		bool tick(SST::Cycle_t x);

		TLBhierarchy() {};
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

		// Setting the link to the memory manager
		bool setOpalLink(SST::Link * l) { to_opal = l ; PTW->set_ToOpal(l); return true;}

		// Setting the line size for the page table walker's dummy requests
		void setLineSize(uint64_t size) { PTW->setLineSize(size); }

	};

}}

#endif
