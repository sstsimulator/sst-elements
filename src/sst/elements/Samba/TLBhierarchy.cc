// Copyright 2009-2016 Sandia Corporation. Under the terms
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


#include <sst_config.h>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
//#include "sst_config.h"
//
#include "sst/core/element.h"

#include <sst/core/interfaces/stringEvent.h>

#include "TLBhierarchy.h"

#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/core/simulation.h>

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




	std::string LEVEL = std::to_string(1);
	std::string cpu_clock = params.find<std::string>("clock", "1GHz");



	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "%" PRIu32, coreID);


	total_waiting = owner->registerStatistic<uint64_t>( "total_waiting", subID );	
	// Initiating all levels of this hierarcy
	TLB * prev=NULL;
	for(int level=levels; level >= 1; level--)
	{
		TLB_CACHE[level] = new TLB(coreID, prev, level, owner, params);
		prev = TLB_CACHE[level];

	}

	for(int level=2; level <=levels; level++)
		TLB_CACHE[level]->setServiceBack(TLB_CACHE[level-1]->getPushedBack());



	TLB_CACHE[1]->setServiceBack(&mem_reqs);

//	registerClock( cpu_clock, new Clock::Handler<TLBhierarchy>(this, &TLBhierarchy::tick ) );
	curr_time = 0;

}




TLBhierarchy::TLBhierarchy(ComponentId_t id, Params& params, int tlb_id) 
{

	coreID=tlb_id;


	std::string cpu_clock = params.find<std::string>("clock", "1GHz");

}



// For now, implement to return a dummy address
long long int TLBhierarchy::translate(long long int VA) { return 0;}


void TLBhierarchy::handleEvent_CACHE( SST::Event * ev ) {

	// Once we receive a response from the cache hierarchy, we just pass it directly to CPU
	to_cpu->send(ev);

}

void TLBhierarchy::handleEvent_CPU(SST::Event* event) {


	// Push the request to the L1 TLB and time-stamp it
	time_tracker[event]= curr_time;
	TLB_CACHE[1]->push_request(event);


}


bool TLBhierarchy::tick(SST::Cycle_t x)
{

	for(int level=levels; level >= 1; level--)
	{
		TLB_CACHE[level]->tick(x);

	}


	curr_time = x;
	// Step 1, check if not empty, then propogate it to L1 TLB
	while(!mem_reqs.empty())
	{


		//		std::cout<<"We have just completed a translation at "<<x<<std::endl;
		SST::Event * event= mem_reqs.back();
		
		if(time_tracker.find(event)==time_tracker.end())
		   std::cout<<"Something wrong happened"<<std::endl;

		uint64_t time_diff = (uint64_t ) x - time_tracker[event];
		time_tracker.erase(event);
		total_waiting->addData(time_diff);
		to_cache->send(event);
		mem_reqs.pop_back();

		// Take it and submit it to L1 TLB, then retire


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

