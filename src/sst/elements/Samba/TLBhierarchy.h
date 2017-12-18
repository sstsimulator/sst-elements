/// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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


		// This is the link to propogate requests to the cache hierarchy
		SST::Component * Owner;


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

                // This tells TLB hierarchy to stall due to emulated page fault or TLB Shootdown
                int hold;


		// This vector holds the current requests to be translated
		std::map<SST::Event *, long long int> mem_reqs_sizes;


		// This mapping is used to track the time spent of translating each request
		std::map<SST::Event *, uint64_t> time_tracker;
		// The access latency in ns
		int latency; 

		// This represents the maximum number of outstanding requests for this structure
		int max_outstanding; 

		public:


		void finish();

		// Handling Events arriving from Cache, mostly responses from memory hierarchy
		void handleEvent_CACHE(SST::Event * event);
		// Handling Events arriving from CPU, mostly requests from Ariel
		void handleEvent_CPU(SST::Event * event);
		// Handling Events arriving from Opal, the memory manager
		void handleEvent_OPAL(SST::Event * event);

		// Constructor for component
		TLBhierarchy(ComponentId_t id, Params& params);
		TLBhierarchy(ComponentId_t id, Params& params, int tlb_id);
		TLBhierarchy(int tlb_id,SST::Component * owner);
		TLBhierarchy(int tlb_id, int level, SST::Component * owner, Params& params);

		PageTableWalker * getPTW(){return PTW;}

		bool tick(SST::Cycle_t x);

		TLBhierarchy() {};
		// Doing the translation
		long long int translate(long long int VA);

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
