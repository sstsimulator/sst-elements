// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include <sst/core/interfaces/stringEvent.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/core/simulation.h>

#include "TLBhierarchy.h"

using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
using namespace SST;
using namespace SST::SambaComponent;


TLBhierarchy::TLBhierarchy(ComponentId_t id, int tlb_id) : ComponentExtension(id)
{

	coreID=tlb_id;

	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "%" PRIu32, coreID);

}


TLBhierarchy::TLBhierarchy(ComponentId_t id, int tlb_id, int Levels, Params& params) : ComponentExtension(id)
{

	output = new SST::Output("SambaComponent[@f:@l:@p] ", 1, 0, SST::Output::STDOUT);

	levels = Levels;
	coreID=tlb_id;

	hold = 0;  // Hold is set to 1 by the page table walker due to fault or shootdown, note that since we don't execute page fault handler or TLB shootdown routine on the core, we just stall TLB hierarchy to emulate the performance effect

	shootdown = 0;  // is set to 1 by the page table walker due to shootdown

	std::string LEVEL = std::to_string(1);
	std::string cpu_clock = params.find<std::string>("clock", "1GHz");

	emulate_faults  = ((uint32_t) params.find<uint32_t>("emulate_faults", 0));

	if(emulate_faults)
	{
		page_placement  = ((uint32_t) params.find<uint32_t>("page_placement", 0));

		shootdown_delay = ((uint32_t) params.find<uint32_t>("shootdown_delay", 8000));

		if(0 == coreID)
			shootdown_delay = (shootdown_delay*2);

		page_swapping_delay = ((uint32_t) params.find<uint32_t>("page_swapping_delay", 1000));

		//std::cerr << owner->getName().c_str() << " core: " << coreID << " shoowtdown_delay: " << shootdown_delay << " page swapping delay: " << page_swapping_delay << std::endl;

	}

	if(emulate_faults) {

		//bool found;
		//std::string mem_size = ((std::string) params.find<std::string>("memory", "", found));
		//if (!found) output->fatal(CALL_INFO, -1, "%s, Param not specified: memory_size\n", getName().c_str());
		std::string mem_size = ((std::string) params.find<std::string>("memory", "0"));

		trim(mem_size);
		size_t pos;
		std::string checkstring = "kKmMgGtTpP";
		if ((pos = mem_size.find_first_of(checkstring)) != std::string::npos) {
			pos++;
			if (pos < mem_size.length() && mem_size[pos] == 'B') {
				mem_size.insert(pos, "i");
			}
		}
		UnitAlgebra ua(mem_size);
		memory_size = ua.getRoundedValue();
		//std::cout<< getName().c_str() << " Core: " << coreID << std::hex << " Memory size: " << memory_size << std::endl;

	}

	max_shootdown_width = ((uint32_t) params.find<uint32_t>("max_shootdown_width", 4));

	ptw_confined  = ((uint32_t) params.find<uint32_t>("ptw_confined", 0));

	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "%" PRIu32, coreID);

	PTW = loadComponentExtension<PageTableWalker>(coreID, nullptr, 0, params);

	total_waiting = registerStatistic<uint64_t>( "total_waiting", subID );

	if(levels)
	{
		// Initiating all levels of this hierarcy
		TLB_CACHE[levels] = loadComponentExtension<TLB>(coreID, nullptr, levels, params);
	        TLB_CACHE[levels]->setPTW(PTW);
		TLB * prev=TLB_CACHE[levels];
		for(int level=levels-1; level >= 1; level--)
		{
			TLB_CACHE[level] = loadComponentExtension<TLB>(coreID, (TLB *) prev, level, params);
			prev = TLB_CACHE[level];

		}

		for(int level=2; level <=levels; level++)
		{
			TLB_CACHE[level]->setServiceBack(TLB_CACHE[level-1]->getPushedBack());
			TLB_CACHE[level]->setServiceBackSize(TLB_CACHE[level-1]->getPushedBackSize());

		}

		timeStamp = 0;
		PTW->setServiceBack(TLB_CACHE[levels]->getPushedBack());
		PTW->setServiceBackSize(TLB_CACHE[levels]->getPushedBackSize());

		TLB_CACHE[1]->setServiceBack(&mem_reqs);
		TLB_CACHE[1]->setServiceBackSize(&mem_reqs_sizes);
	}
	else
	{
		PTW->setServiceBack(&mem_reqs);
		PTW->setServiceBackSize(&mem_reqs_sizes);
	}

	PTW->setHold(&hold);
	PTW->setShootDownEvents(&shootdown,&hasInvalidAddrs,&invalid_addrs);
	PTW->setPagePlacement(&page_placement);
	PTW->setTimeStamp(&timeStamp);
	PTW->setLocalMemorySize(&memory_size);

	curr_time = 0;

}




TLBhierarchy::TLBhierarchy(ComponentId_t id, Params& params, int tlb_id) : ComponentExtension(id)
{

	coreID=tlb_id;

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");
}



// For now, implement to return a dummy address
Address_t TLBhierarchy::translate(Address_t VA) { return 0;}


void TLBhierarchy::handleEvent_CACHE( SST::Event * ev )
{

	// Once we receive a response from the cache hierarchy, we just pass it directly to CPU
	to_cpu->send(ev);

}

void TLBhierarchy::handleEvent_CPU(SST::Event* event)
{
	// Push the request to the L1 TLB and time-stamp it
	time_tracker[event]= curr_time;
        MemEventBase* mEvent = static_cast<MemEventBase*>(event);
	TLB_CACHE[1]->push_request(mEvent);


}


bool TLBhierarchy::tick(SST::Cycle_t x)
{
	timeStamp++;

	/*if(emulate_faults)
	{
		f(shootdown && hasInvalidAddrs)
		{
			int dispatched = 0;

			while(!invalid_addrs.empty()){

				if(dispatched >= max_shootdown_width)
					return false;

				std::pair<Address_t, int> addr = invalid_addrs.back();
				//std::cerr << Owner->getName().c_str() << " invalid address: " << addr.first << " level: " << addr.second << std::endl;
				for(int level = levels; level >= 1; level--)
					TLB_CACHE[level]->invalidate(addr.first, addr.second);

				PTW->invalidate(addr.first, addr.second);

				invalid_addrs.pop_back();

				if(invalid_addrs.empty())
					PTW->sendShootdownAck(shootdown_delay,page_swapping_delay);

				dispatched++;
			}

			return false;
		}
	}*/

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
	while(!mem_reqs.empty() && !shootdown && !hold)
	{
            MemHierarchy::MemEventBase * event= mem_reqs.back();

		if(time_tracker.find(event) == time_tracker.end())
		{
			std::cout << "Danger! Something is terribly wrong..." << std::endl;
			mem_reqs_sizes.erase(event);
			mem_reqs.pop_back();
			continue;
		}

		// Here we override the physical address provided by ariel memory manage by the one provided by page fault handler
		if(emulate_faults)
		{
			Address_t vaddr = ((MemEvent*) event)->getVirtualAddress();
			if(!ptw_confined)
			{
				if((*PTE).find(vaddr/4096)==(*PTE).end())
					std::cout<<"Error: That page has never been mapped:  " << vaddr / 4096 << std::endl;

				((MemEvent*) event)->setAddr((((*PTE)[vaddr / 4096] + vaddr % 4096) / 64) * 64);
				((MemEvent*) event)->setBaseAddr((((*PTE)[vaddr / 4096] + vaddr % 4096) / 64) * 64);
			}
			else
			{
				uint64_t offset = (uint64_t)512*512*512*512;
				if((*PTE).find((vaddr/4096)%offset)==(*PTE).end())
				std::cout<<"Error: That page has never been mapped:  " << vaddr / 4096 << std::endl;

				((MemEvent*) event)->setAddr((((*PTE)[(vaddr / 4096)%offset] + vaddr % 4096)));
				((MemEvent*) event)->setBaseAddr((((*PTE)[(vaddr / 4096)%offset] + vaddr % 4096) / 64) * 64);
			}

			/*if(page_placement) {
				if((*PTE)[vaddr / 4096] < memory_size ) {
					//if((*PTR_map).find((*PTE)[vaddr / 4096]) == (*PTR_map).end())
					//	std::cout<<"Error: page reference is not mapped, vaddress:" << vaddr / 4096 << " physical page: " << (*PTE)[vaddr / 4096] << " PTE " << std::endl;

					//std::cerr << Owner->getName().c_str() << " Page table reference update address: " << (*PTR)[(*PTR_map)[(*PTE)[vaddr / 4096]]].first << " index: " <<
					//		(*PTR_map)[(*PTE)[vaddr / 4096]] << " PTE " << std::endl;
					PTW->updatePTR((*PTE)[vaddr / 4096]);

					//(*PTR)[(*PTE)[vaddr / 4096]] = 1;
				}
			}*/

		}

		uint64_t time_diff = (uint64_t ) x - time_tracker[event];
		time_tracker.erase(event);
		total_waiting->addData(time_diff);

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

