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
#include "Samba.h"


using namespace SST::Interfaces;
using namespace SST;
using namespace SST::SambaComponent;


#define SAMBA_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT


// Here we do the initialization of the Samba units of the system, connecting them to the cores and instantiating TLB hierachy objects for each one

Samba::Samba(SST::ComponentId_t id, SST::Params& params): Component(id) {

	core_count = (uint32_t) params.find<uint32_t>("corecount", 1);

	int levels = (uint32_t) params.find<uint32_t>("levels", 1);



	TLB = new TLBhierarchy*[core_count];
	std::cout<<"Initialized with "<<core_count<<" cores"<<std::endl;






	cpu_to_mmu = (SST::Link **) malloc( sizeof(SST::Link*) * core_count );

	mmu_to_cache = (SST::Link **) malloc( sizeof(SST::Link *) * core_count );

	char* link_buffer = (char*) malloc(sizeof(char) * 256);

	char* link_buffer2 = (char*) malloc(sizeof(char) * 256);




	std::cout<<"Before initialization "<<std::endl;
	for(uint32_t i = 0; i < core_count; ++i) {
		sprintf(link_buffer, "cpu_to_mmu%" PRIu32, i);

		sprintf(link_buffer2, "mmu_to_cache%" PRIu32, i);



		TLB[i]= new TLBhierarchy(i, levels /* level */, (SST::Component *) this,params);


		SST::Link * link2 = configureLink(link_buffer, "0ps", new Event::Handler<TLBhierarchy>(TLB[i], &TLBhierarchy::handleEvent_CPU));
		cpu_to_mmu[i] = link2;



		SST::Link * link = configureLink(link_buffer2, "0ps", new Event::Handler<TLBhierarchy>(TLB[i], &TLBhierarchy::handleEvent_CACHE));
		mmu_to_cache[i] = link;


		TLB[i]->setCacheLink(mmu_to_cache[i]);
		TLB[i]->setCPULink(cpu_to_mmu[i]);

	}

	std::cout<<"After initialization "<<std::endl;

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");
	registerClock( cpu_clock, new Clock::Handler<Samba>(this, &Samba::tick ) );



	free(link_buffer);
	free(link_buffer2);

}



Samba::Samba() : Component(-1)
{
	// for serialization only
	// 
}



bool Samba::tick(SST::Cycle_t x)
{

	for(uint32_t i = 0; i < core_count; ++i)
		TLB[i]->tick(x);



}
