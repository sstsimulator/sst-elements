// Copyright 2009-2018 NTESS. Under the terms
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


#include <sst_config.h>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/element.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/core/simulation.h>

#include "TLBhierarchy.h"

using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
using namespace SST;
using namespace SST::SambaComponent;


TLBhierarchy::TLBhierarchy(int tlb_id, SST::Component * owner)
{

	Owner = owner;

	coreID=tlb_id;

	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "%" PRIu32, coreID);

}


TLBhierarchy::TLBhierarchy(int tlb_id, int Levels, SST::Component * owner, Params& params)
{

	levels = Levels;
	Owner = owner;
	coreID=tlb_id;

	hold = 0;  // Hold is set to 1 by the page table walker due to fault or shootdown, note that since we don't execute page fault handler or TLB shootdown routine on the core, we just stall TLB hierarchy to emulate the performance effect

	std::string LEVEL = std::to_string(1);
	std::string cpu_clock = params.find<std::string>("clock", "1GHz");

	emulate_faults  = ((uint32_t) params.find<uint32_t>("emulate_faults", 0));

	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "%" PRIu32, coreID);

	PTW = new PageTableWalker(coreID, NULL, 0, owner, params);

	total_waiting = owner->registerStatistic<uint64_t>( "total_waiting", subID );	

	// Initiating all levels of this hierarcy
	TLB_CACHE[levels] = new TLB(coreID, PTW, levels, owner, params);
	TLB * prev=TLB_CACHE[levels];
	for(int level=levels-1; level >= 1; level--)
	{
		TLB_CACHE[level] = new TLB(coreID, (TLB *) prev, level, owner, params);
		prev = TLB_CACHE[level];

	}

	for(int level=2; level <=levels; level++)
	{
		TLB_CACHE[level]->setServiceBack(TLB_CACHE[level-1]->getPushedBack());	
		TLB_CACHE[level]->setServiceBackSize(TLB_CACHE[level-1]->getPushedBackSize());

	}


	PTW->setServiceBack(TLB_CACHE[levels]->getPushedBack());
	PTW->setServiceBackSize(TLB_CACHE[levels]->getPushedBackSize());
	PTW->setHold(&hold);

	TLB_CACHE[1]->setServiceBack(&mem_reqs);
	TLB_CACHE[1]->setServiceBackSize(&mem_reqs_sizes);


	curr_time = 0;

}




TLBhierarchy::TLBhierarchy(ComponentId_t id, Params& params, int tlb_id) 
{

	coreID=tlb_id;

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");
}



// For now, implement to return a dummy address
long long int TLBhierarchy::translate(long long int VA) { return 0;}


void TLBhierarchy::handleEvent_CACHE( SST::Event * ev )
{

	// Once we receive a response from the cache hierarchy, we just pass it directly to CPU
	to_cpu->send(ev);

}

void TLBhierarchy::handleEvent_CPU(SST::Event* event)
{
	// Push the request to the L1 TLB and time-stamp it
	time_tracker[event]= curr_time;
	TLB_CACHE[1]->push_request(event);


}


bool TLBhierarchy::tick(SST::Cycle_t x)
{
	if(hold==1) // If the page table walker is suggesting to hold, the TLBhierarchy will stop processing any events
	{
		PTW->tick(x);
		return false;
	}

	for(int level = levels; level >= 1; level--)
	{
		TLB_CACHE[level]->tick(x);
	}

	PTW->tick(x);

	curr_time = x;
	// Step 1, check if not empty, then propogate it to L1 cache
	while(!mem_reqs.empty())
	{
		SST::Event * event= mem_reqs.back();

		if(time_tracker.find(event) == time_tracker.end())
		{ 
			std::cout << "Danger! Something is terribly wrong..." << std::endl;
			mem_reqs_sizes.erase(event);
			mem_reqs.pop_back();
			continue;
		}

		uint64_t time_diff = (uint64_t ) x - time_tracker[event];
		time_tracker.erase(event);
		total_waiting->addData(time_diff);
		
		// Here we override the physical address provided by ariel memory manage by the one provided by Opal
		if(emulate_faults)
		{
			long long int vaddr = ((MemEvent*) event)->getVirtualAddress();
			if((*PTE).find(vaddr/4096)==(*PTE).end())
				std::cout<<"Error: That page has never been mapped:  " << vaddr / 4096 << std::endl;

			((MemEvent*) event)->setAddr((((*PTE)[vaddr / 4096] + vaddr % 4096) / 64) * 64);
			((MemEvent*) event)->setBaseAddr((((*PTE)[vaddr / 4096] + vaddr % 4096) / 64) * 64);
		}
		to_cache->send(event);

		// We remove the size of that translation, we might for future versions use the translation size to obtain statistics
		mem_reqs_sizes.erase(event);
		mem_reqs.pop_back();
	}

	return false;
}


void TLBhierarchy::finish()
{
      for(int level=1; level<=levels;level++)
	{
		TLB_CACHE[level]->finish();
	}

}

