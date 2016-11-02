/// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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

		// This defines the size of the TLB in number of entries, regardless of the type (e.g., page size) of entry
		int size;

		// This is the link to propogate requests to the cache hierarchy
		SST::Component * Owner;


		SST::Link * to_cache;


		SST::Link * to_cpu;


		std::map<int, TLB *> TLB_CACHE;

		std::string clock_frequency_str;

		// Holds the current time 
		SST::Cycle_t curr_time;

		// This vector holds the current requests to be translated
		std::vector<SST::Event *> mem_reqs;


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
		// Constructor for component


		TLBhierarchy(ComponentId_t id, Params& params);
		TLBhierarchy(ComponentId_t id, Params& params, int tlb_id);
		TLBhierarchy(int tlb_id,SST::Component * owner);
		TLBhierarchy(int tlb_id, int level, SST::Component * owner, Params& params);


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




	};

}}

#endif
