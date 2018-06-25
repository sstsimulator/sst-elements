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

/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */



#include <sst_config.h>
#include <string>
#include "Samba.h"


using namespace SST::Interfaces;
using namespace SST;
using namespace SST::SambaComponent;


#define SAMBA_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT


// Here we do the initialization of the Samba units of the system, connecting them to the cores and instantiating TLB hierachy objects for each one

Samba::Samba(SST::ComponentId_t id, SST::Params& params): Component(id) {

	//CR3 = -1;

	core_count = (uint32_t) params.find<uint32_t>("corecount", 1);

	int self = (uint32_t) params.find<uint32_t>("self_connected", 0);

	int emulate_faults = (uint32_t) params.find<uint32_t>("emulate_faults", 0);

	int levels = (uint32_t) params.find<uint32_t>("levels", 1);

	int  page_walk_latency = ((uint32_t) params.find<uint32_t>("page_walk_latency", 50));

	TLB = new TLBhierarchy*[core_count];
	std::cout<<"Initialized with "<<core_count<<" cores"<<std::endl;


	cpu_to_mmu = (SST::Link **) malloc( sizeof(SST::Link*) * core_count );

	mmu_to_cache = (SST::Link **) malloc( sizeof(SST::Link *) * core_count );

	ptw_to_mem = (SST::Link **) malloc( sizeof(SST::Link *) * core_count );

	ptw_to_opal = (SST::Link **) malloc( sizeof(SST::Link *) * core_count );


	char* link_buffer = (char*) malloc(sizeof(char) * 256);

	char* link_buffer2 = (char*) malloc(sizeof(char) * 256);

	char* link_buffer3 = (char*) malloc(sizeof(char) * 256);

	char* link_buffer4 = (char*) malloc(sizeof(char) * 256);

	std::cout<<"Before initialization "<<std::endl;
	for(uint32_t i = 0; i < core_count; ++i) {
		sprintf(link_buffer, "cpu_to_mmu%" PRIu32, i);

		sprintf(link_buffer2, "mmu_to_cache%" PRIu32, i);



		TLB[i]= new TLBhierarchy(i, levels /* level */, (SST::Component *) this,params);


		SST::Link * link2 = configureLink(link_buffer, "0ps", new Event::Handler<TLBhierarchy>(TLB[i], &TLBhierarchy::handleEvent_CPU));
		cpu_to_mmu[i] = link2;



		SST::Link * link = configureLink(link_buffer2, "0ps", new Event::Handler<TLBhierarchy>(TLB[i], &TLBhierarchy::handleEvent_CACHE));
		mmu_to_cache[i] = link;


		sprintf(link_buffer3, "ptw_to_mem%" PRIu32, i);
		SST::Link * link3;

		if(self==0)
			link3 = configureLink(link_buffer3, new Event::Handler<PageTableWalker>(TLB[i]->getPTW(), &PageTableWalker::recvResp));
		else
			link3 = configureSelfLink(link_buffer3, std::to_string(page_walk_latency)+ "ns", new Event::Handler<PageTableWalker>(TLB[i]->getPTW(), &PageTableWalker::recvResp));

		ptw_to_mem[i] = link3;


		sprintf(link_buffer4, "ptw_to_opal%" PRIu32, i);
		SST::Link * link4;


		if(emulate_faults==1)
		{
			link4 = configureLink(link_buffer4, new Event::Handler<PageTableWalker>(TLB[i]->getPTW(), &PageTableWalker::recvOpal));
			ptw_to_opal[i] = link4;
			TLB[i]->setOpalLink(link4);

			sprintf(link_buffer, "event_bus%" PRIu32, i);

			event_link = configureSelfLink(link_buffer, "1ns", new Event::Handler<PageTableWalker>(TLB[i]->getPTW(), &PageTableWalker::handleEvent));

			TLB[i]->getPTW()->setEventChannel(event_link);
			TLB[i]->setPageTablePointers(&CR3, &PGD, &PUD, &PMD, &PTE, &MAPPED_PAGE_SIZE1GB, &MAPPED_PAGE_SIZE2MB, &MAPPED_PAGE_SIZE4KB, &PENDING_PAGE_FAULTS, &PENDING_SHOOTDOWN_EVENTS);

		}

		TLB[i]->setPTWLink(link3); // We make a connection from the TLB hierarchy to the memory hierarchy, for page table walking purposes
		TLB[i]->setCacheLink(mmu_to_cache[i]);
		TLB[i]->setCPULink(cpu_to_mmu[i]);

	}



	std::cout<<"After initialization "<<std::endl;

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");
	registerClock( cpu_clock, new Clock::Handler<Samba>(this, &Samba::tick ) );



	free(link_buffer);
	free(link_buffer2);
	free(link_buffer3);

}



Samba::Samba() : Component(-1)
{
	// for serialization only
	// 
}


void Samba::init(unsigned int phase) {

	/* 
	 * CPU may send init events to memory, pass them through, 
	 * also pass memory events to CPU
	 */
	for (uint32_t i = 0; i < core_count; i++) {
		SST::Event * ev;
		while ((ev = cpu_to_mmu[i]->recvInitData())) {
			mmu_to_cache[i]->sendInitData(ev);
		}

		while ((ev = mmu_to_cache[i]->recvInitData())) {
			SST::MemHierarchy::MemEventInit * mEv = dynamic_cast<SST::MemHierarchy::MemEventInit*>(ev);
			if (mEv && mEv->getInitCmd() == SST::MemHierarchy::MemEventInit::InitCommand::Coherence) {
				SST::MemHierarchy::MemEventInitCoherence * mEvC = static_cast<SST::MemHierarchy::MemEventInitCoherence*>(mEv);
				TLB[i]->setLineSize(mEvC->getLineSize()); 
			}
			cpu_to_mmu[i]->sendInitData(ev);
		}
	}

}


bool Samba::tick(SST::Cycle_t x)
{

	// We tick the MMU hierarchy of each core
	for(uint32_t i = 0; i < core_count; ++i)
		TLB[i]->tick(x);


	return false;
}
