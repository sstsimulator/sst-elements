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
#include "PageTableWalker.h"
#include <sst/core/link.h>
#include <sst/elements/Opal/Opal_Event.h>
#include "Samba_Event.h"
#include<iostream>

using namespace SST::SambaComponent;
using namespace SST::OpalComponent;
using namespace SST::MemHierarchy;
using namespace SST;




int max(int a, int b)
{

	if ( a > b )
		return a;
	else
		return b;


}

int abs_int_Samba(int x)
{

	if(x<0)
		return -1*x;
	else
		return x;

}


PageTableWalker::PageTableWalker(int Page_size, int Assoc, PageTableWalker * Next_level, int Size)
{


}


PageTableWalker::PageTableWalker(int tlb_id, PageTableWalker * Next_level, int level, SST::Component * owner, SST::Params& params)
{

	output = new SST::Output("OpalComponent[@f:@l:@p] ", 1, 0, SST::Output::STDOUT);

	coreId = tlb_id;

	fault_level = 0;

	std::string LEVEL = std::to_string(level);

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");

	stall = false; // No stall initially; no page faults


	self_connected = ((uint32_t) params.find<uint32_t>("self_connected", 0));

	page_walk_latency = ((uint32_t) params.find<uint32_t>("page_walk_latency", 50));

	max_outstanding = ((uint32_t) params.find<uint32_t>("max_outstanding_PTWC", 4));

	latency = ((uint32_t) params.find<uint32_t>("latency_PTWC", 1));

	to_mem = NULL;

	hold_for_mem = 0;

	active_PTW = ((uint32_t) params.find<uint32_t>("active_TLB", 1));

	emulate_faults  = ((uint32_t) params.find<uint32_t>("emulate_faults", 0));

	if(emulate_faults)
	{
		opal_latency = ((uint32_t) params.find<uint32_t>("page_walk_to_opal_latency", 2000));	//2us

		//if(*page_placement == 1)
		//	PTR_REF_POINTER = 0;
	}

	// For now, we assume a typicaly x86-64 system with 4 levels of page table
	sizes = 4;

	parallel_mode = ((uint32_t) params.find<uint32_t>("parallel_mode_L"+LEVEL, 0));

	upper_link_latency = ((uint32_t) params.find<uint32_t>("upper_link_L"+LEVEL, 0));


	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "Core%d_PTWC", tlb_id);


	// The stats that will appear, not that these stats are going to be part of the Samba unit
	statPageTableWalkerHits = owner->registerStatistic<uint64_t>( "tlb_hits", subID);
	statPageTableWalkerMisses = owner->registerStatistic<uint64_t>( "tlb_misses", subID );

	max_width = ((uint32_t) params.find<uint32_t>("max_width_PTWC", 4));

	Owner = owner;
	os_page_size = ((uint32_t) params.find<uint32_t>("os_page_size", 4));

	size = new int[sizes]; 
	assoc = new int[sizes];
	page_size = new uint64_t[sizes];
	sets = new int[sizes];
	tags = new Address_t**[sizes];
	valid = new bool**[sizes];
	lru = new int **[sizes];


	// page table offsets
	page_size[0] = 1024*4;
	page_size[1] = 512*1024*4;
	page_size[2] = (uint64_t) 512*512*1024*4;
	page_size[3] = (uint64_t) 512*512*512*1024*4;

	for(int i=0; i < sizes; i++)
	{

		size[i] =  ((uint32_t) params.find<uint32_t>("size"+std::to_string(i+1) + "_PTWC", 1));

		assoc[i] =  ((uint32_t) params.find<uint32_t>("assoc"+std::to_string(i+1) +  "_PTWC", 1));

		// We define the number of sets for that structure of page size number i
		sets[i] = size[i]/assoc[i];

	}

	hits=misses=0;



	for(int id=0; id< sizes; id++)
	{

		tags[id] = new Address_t*[sets[id]];

		valid[id] = new bool*[sets[id]];

		lru[id] = new int*[sets[id]];

		for(int i=0; i < sets[id]; i++)
		{
			tags[id][i]=new Address_t[assoc[id]];
			valid[id][i]=new bool[assoc[id]];
			lru[id][i]=new int[assoc[id]];
			for(int j=0; j<assoc[id];j++)
			{
				tags[id][i][j]=0;
				valid[id][i][j]=false;
				lru[id][i][j]=j;
			}
		}

	}

	sprintf(subID, "%" PRIu32, coreId);

/*
	tlb_shootdown = owner->registerStatistic<uint64_t>( "tlb_shootdown", subID );
	tlb_shootdown_delay = owner->registerStatistic<uint64_t>( "tlb_shootdown_delay", subID );
	pgd_accesses = owner->registerStatistic<uint64_t>( "pgd_mem_accesses", subID );
	pud_accesses = owner->registerStatistic<uint64_t>( "pud_mem_accesses", subID );
	pmd_accesses = owner->registerStatistic<uint64_t>( "pmd_mem_accesses", subID );
	pte_accesses = owner->registerStatistic<uint64_t>( "pte_mem_accesses", subID );

	pgd_unique_access = owner->registerStatistic<uint64_t>( "pgd_unique_accesses", subID );
	pud_unique_access = owner->registerStatistic<uint64_t>( "pud_unique_accesses", subID );
	pmd_unique_access = owner->registerStatistic<uint64_t>( "pmd_unique_accesses", subID );
	pte_unique_access = owner->registerStatistic<uint64_t>( "pte_unique_accesses", subID );

	cr3_addresses_allocated = owner->registerStatistic<uint64_t>( "cr3_addresses_allocated", subID );
	pgd_addresses_allocated_local = owner->registerStatistic<uint64_t>( "pgd_pages_local", subID );
	pud_addresses_allocated_local = owner->registerStatistic<uint64_t>( "pud_pages_local", subID );
	pmd_addresses_allocated_local = owner->registerStatistic<uint64_t>( "pmd_pages_local", subID );
	pte_addresses_allocated_local = owner->registerStatistic<uint64_t>( "pte_pages_local", subID );
	pgd_addresses_allocated_global = owner->registerStatistic<uint64_t>( "pgd_pages_global", subID );
	pud_addresses_allocated_global = owner->registerStatistic<uint64_t>( "pud_pages_global", subID );
	pmd_addresses_allocated_global = owner->registerStatistic<uint64_t>( "pmd_pages_global", subID );
	pte_addresses_allocated_global = owner->registerStatistic<uint64_t>( "pte_pages_global", subID );

	pgd_addresses_migrated = owner->registerStatistic<uint64_t>( "pgd_addresses_migrated", subID );
	pud_addresses_migrated = owner->registerStatistic<uint64_t>( "pud_addresses_migrated", subID );
	pmd_addresses_migrated = owner->registerStatistic<uint64_t>( "pmd_addresses_migrated", subID );
	pte_addresses_migrated = owner->registerStatistic<uint64_t>( "pte_addresses_migrated", subID );
	num_of_pages_migrated = owner->registerStatistic<uint64_t>( "pages_migrated", subID );
*/

}

// Handling internal events that are sent by the Page Table Walker
void PageTableWalker::handleEvent( SST::Event* e )
{


	// For each page fault,
	// 1 -- check if CR3 for VA exist, if not
	// Send request to Opal and save state of current fault at 3
	// one recvOpal is called due to response from Opal, recvOpal will create an event that will trigger HandleEvent
	// If HandleEvent comes and state is 3, it again changes state to 2 and send a request to Opal...
	//
	// Finally, one a physical page is assigned, the stall is set to false, and *hold is reset



	//// ******** Important, for each level, we will update MAPPED_PAGE_SIZE** and PGD/PMD/PUD/PTE so the actual physical address is used later
	SambaEvent * temp_ptr =  dynamic_cast<SambaComponent::SambaEvent*> (e);

	if(temp_ptr==NULL)
		std::cout<<" Error in Casting to SambaEvent "<<std::endl;

	if(temp_ptr->getType() == EventType::PAGE_FAULT)
	{

		// Send request to Opal starting from the first unmapped level (L4/CR3 if first fault in system)

		OpalEvent * tse = new OpalEvent(OpalComponent::EventType::REQUEST);
		uint64_t offset = (uint64_t)512*512*512*512;
		if(to_mem!=NULL) {
		//if((*CR3) == -1)
		if(!(*cr3_init))
			fault_level = 4;
		else if((*PGD).find((temp_ptr->getAddress()/page_size[3])%512) == (*PGD).end())
			fault_level = 3;
		else if((*PUD).find((temp_ptr->getAddress()/page_size[2])%(512*512)) == (*PUD).end())
			fault_level = 2;
		else if((*PMD).find((temp_ptr->getAddress()/page_size[1])%(512*512*512)) == (*PMD).end())
			fault_level = 1;
		else if((*PTE).find((temp_ptr->getAddress()/page_size[0])%offset) == (*PTE).end())
			fault_level = 0;
		else
			output->fatal(CALL_INFO, -1, "MMU: DANGER!!\n");

		if(!(*cr3_init)) {
			*cr3_init = 1;
			tse->setResp(stall_addr,0,4096);
		}
		else
			tse->setResp(stall_addr/page_size[fault_level],0,4096);
		}
		else {
			fault_level = 0;
			tse->setResp(stall_addr/page_size[fault_level],0,4096);
		}
		tse->setFaultLevel(fault_level);
		to_opal->send(tse);



	}
	else if(temp_ptr->getType() == EventType::OPAL_RESPONSE)
	{

		// If opal response and it is the lst, i.e., PTE, then just do the following:
		// (1) update page table and maps of mapped pages -- (2) stall = false, *hold=0
		// Otherwise
		// Update the page tables to reflect new page table entries/tables, then issue a new opal request to build next level

		// For now, just assume only the page will be mappe and requested from Opal
		if(fault_level == 4)
		{
			// We are building the first page in the page table!
			//std::cerr << Owner->getName().c_str() << " Core: " << coreId << " CR3 address: " << std::hex << temp_ptr->getPaddress() << " cr3_init: " << *cr3_init<< " hold: "<< *hold <<std::endl;
			*cr3_init = 1;
			(*CR3) = temp_ptr->getPaddress();
			//if(temp_ptr->getPaddress() < *local_memory_size) pgd_pages_allocated_at_local->addData(1);
			//else pgd_pages_allocated_at_global->addData(1);
			fault_level--;
			OpalEvent * tse = new OpalEvent(OpalComponent::EventType::REQUEST);
			tse->setResp(stall_addr/page_size[fault_level],0,4096);
			tse->setFaultLevel(fault_level);
			to_opal->send(opal_latency,tse);
			//cr3_addresses_allocated->addData(1);

		}
		else if(fault_level == 3)
		{
			//std::cerr << Owner->getName().c_str() << " Core: " << coreId << " PGD stall_addr: " << stall_addr << " vaddress: " << std::hex << temp_ptr->getAddress() << " paddress: " << temp_ptr->getPaddress() << "offset: " << (stall_addr/page_size[3])%512 << " hold: "<< *hold<< std::endl;
			if((*PGD).find((stall_addr/page_size[3])%512) != (*PGD).end())
				output->fatal(CALL_INFO, -1, "MMU: PTW DANGER.. same PGD!!\n");
			(*PGD)[(stall_addr/page_size[3])%512] = temp_ptr->getPaddress();
			(*PENDING_PAGE_FAULTS_PGD).erase((stall_addr/page_size[3])%(512));
			//if(temp_ptr->getPaddress() < *local_memory_size) {
			//	(*PTR_map)[temp_ptr->getPaddress()] = (*PTR).size();
			//	(*PTR).push_back(std::make_pair(temp_ptr->getPaddress(),1));
			//}
		//	if(temp_ptr->getPaddress() < *local_memory_size) pgd_addresses_allocated_local->addData(1);
		//	else pgd_addresses_allocated_global->addData(1);
			//if(temp_ptr->getPaddress() < *local_memory_size) pud_pages_allocated_at_local->addData(1);
			//else pud_pages_allocated_at_global->addData(1);
			fault_level--;
			OpalEvent * tse = new OpalEvent(OpalComponent::EventType::REQUEST);
			tse->setResp(stall_addr/page_size[fault_level],0,4096);
			tse->setFaultLevel(fault_level);
			to_opal->send(opal_latency,tse);
			//pgd_addresses_allocated->addData(1);

		}
		else if(fault_level == 2)
		{
			//std::cerr << Owner->getName().c_str() << " Core: " << coreId << " PUD stall_addr: " << stall_addr << " vaddress: " << std::hex << temp_ptr->getAddress() << " paddress: " << temp_ptr->getPaddress() << "offset: " << (stall_addr/page_size[2])%(512*512) << " hold: "<< *hold<< std::endl;
                        if((*PUD).find((stall_addr/page_size[2])%(512*512)) != (*PUD).end())
                                output->fatal(CALL_INFO, -1, "MMU: PTW DANGER.. same PUD!!\n");
			(*PUD)[(stall_addr/page_size[2])%(512*512)] = temp_ptr->getPaddress();
			(*PENDING_PAGE_FAULTS_PUD).erase((stall_addr/page_size[2])%(512*512));
			//if(temp_ptr->getPaddress() < *local_memory_size) {
			//	(*PTR_map)[temp_ptr->getPaddress()] = (*PTR).size();
			//	(*PTR).push_back(std::make_pair(temp_ptr->getPaddress(),1));
			//}
			//if(temp_ptr->getSize() == page_size[2]) {
			//	(*MAPPED_PAGE_SIZE1GB)[temp_ptr->getAddress()/page_size[2]] = 0;
			//	fault_level = 0;
			//	stall = false;
			//	*hold = 0;

			//} else {
		//	if(temp_ptr->getPaddress() < *local_memory_size) pud_addresses_allocated_local->addData(1);
		//	else pud_addresses_allocated_global->addData(1);
			//if(temp_ptr->getPaddress() < *local_memory_size) pmd_pages_allocated_at_local->addData(1);
			//else pmd_pages_allocated_at_global->addData(1);
				fault_level--;
				OpalEvent * tse = new OpalEvent(OpalComponent::EventType::REQUEST);
				tse->setResp(stall_addr/page_size[fault_level],0,4096);
				tse->setFaultLevel(fault_level);
				to_opal->send(opal_latency,tse);
				//pud_addresses_allocated->addData(1);
			//}

		}

		else if(fault_level == 1)
		{
			uint64_t offset = 512*512*512;
			//std::cerr << Owner->getName().c_str() << " Core: " << coreId << " PMD stall_addr: " << stall_addr << " vaddress: " << std::hex << temp_ptr->getAddress() << " paddress: " << temp_ptr->getPaddress()<< "offset: " << (stall_addr/page_size[1])%offset << " hold: "<< *hold<< std::endl;
			//uint64_t offset = 512*512*512;
                        if((*PMD).find((stall_addr/page_size[1])%offset) != (*PMD).end())
                                output->fatal(CALL_INFO, -1, "MMU: PTW DANGER.. same PMD!!\n");
			(*PMD)[(stall_addr/page_size[1])%offset] = temp_ptr->getPaddress();
			(*PENDING_PAGE_FAULTS_PMD).erase((stall_addr/page_size[1])%offset);
			//if(temp_ptr->getPaddress() < *local_memory_size) {
			//	(*PTR_map)[temp_ptr->getPaddress()] = (*PTR).size();
			//	(*PTR).push_back(std::make_pair(temp_ptr->getPaddress(),1));
			//}
			//if(temp_ptr->getSize() == page_size[1]) {
			//	(*MAPPED_PAGE_SIZE2MB)[temp_ptr->getAddress()/page_size[1]] = 0;
			//	fault_level = 0;
			//	stall = false;
			//	*hold = 0;

			//} else {
//			if(temp_ptr->getPaddress() < *local_memory_size) pmd_addresses_allocated_local->addData(1);
//			else pmd_addresses_allocated_global->addData(1);
			//if(temp_ptr->getPaddress() < *local_memory_size) pte_pages_allocated_at_local->addData(1);
			//else pte_pages_allocated_at_global->addData(1);
				fault_level--;
				OpalEvent * tse = new OpalEvent(OpalComponent::EventType::REQUEST);
				tse->setResp(stall_addr/page_size[fault_level],0,4096);
				tse->setFaultLevel(fault_level);
				to_opal->send(opal_latency,tse);
				//pmd_addresses_allocated->addData(1);

			//}

		}
		else if(fault_level == 0)
		{
			//std::cerr << Owner->getName().c_str() << " Core: " << coreId << " PTE stall_addr: " << stall_addr << " vaddress: " << std::hex << temp_ptr->getAddress() << " paddress: " << temp_ptr->getPaddress() << " hold: "<< *hold<< std::endl;
			uint64_t offset = (uint64_t)512*512*512*512;
                        if((*PTE).find((stall_addr/page_size[0])%offset) != (*PTE).end())
                                output->fatal(CALL_INFO, -1, "MMU: PTW DANGER.. same PTE!!\n");
			(*PTE)[(stall_addr/page_size[0])%offset] = temp_ptr->getPaddress();
			//if(temp_ptr->getPaddress() < *local_memory_size) {
			//	(*PTR_map)[temp_ptr->getPaddress()] = (*PTR).size();
			//	(*PTR).push_back(std::make_pair(temp_ptr->getPaddress(),1));
			//}
//			if(temp_ptr->getPaddress() < *local_memory_size) pte_addresses_allocated_local->addData(1);
//			else pte_addresses_allocated_global->addData(1);
			//if(temp_ptr->getPaddress() < *local_memory_size) nonPT_pages_allocated_at_local->addData(1);
			//else nonPT_pages_allocated_at_global->addData(1);
			//pte_addresses_allocated->addData(1);
			SambaEvent * tse = new SambaEvent(EventType::PAGE_FAULT_SERVED);
			s_EventChan->send(opal_latency,tse);
		}


	}
	/*else if(temp_ptr->getType() == EventType::SDACK)
	{
		//PAGE_REFERENCE_TABLE->reset();
		OpalEvent * tse = new OpalEvent(OpalComponent::EventType::SDACK);
		tse->setSize(shootdownId);
		to_opal->send(tse);
		*shootdown=0;
		tlb_shootdown_delay->addData(*timeStamp - tlb_shootdown_time);
		//std::cerr << Owner->getName().c_str() << " core: " << coreId << " shootdown delay: " << *timeStamp - tlb_shootdown_time << " currTime: " << *timeStamp << " shootdown time: " << tlb_shootdown_time << std::endl;
		tlb_shootdown_time = 0;
	}
	else if(temp_ptr->getType() == EventType::PAGE_REFERENCE)
	{
		OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
		tse->setResp(temp_ptr->getAddress(),temp_ptr->getPaddress(),temp_ptr->getSize());
		to_opal->send(tse);
	}*/
	else if(temp_ptr->getType() == EventType::PAGE_FAULT_SERVED)
	{
		uint64_t offset = (uint64_t)512*512*512*512;
		(*MAPPED_PAGE_SIZE4KB)[(stall_addr/page_size[0])%offset] = 0;
		(*PENDING_PAGE_FAULTS_PTE).erase((stall_addr/page_size[0])%offset);
	}
	delete temp_ptr;
}




void PageTableWalker::recvOpal(SST::Event * event)
{

	OpalEvent * ev =  dynamic_cast<OpalComponent::OpalEvent*> (event);

	switch(ev->getType()) {
	case SST::OpalComponent::EventType::RESPONSE:
	{
		SambaEvent * tse = new SambaEvent(EventType::OPAL_RESPONSE);
		tse->setResp(ev->getAddress(), ev->getPaddress(),4096);
		s_EventChan->send(tse);
	}
	break;

	/*case SST::OpalComponent::EventType::PAGE_REFERENCE:
	{
		for(int i=0; i<PAGE_REFERENCE_TABLE->size && cr3_init == 1; i++)
		{
			if( PAGE_REFERENCE_TABLE->page_reference[i].first != -1 )
			{
				//std::cerr << Owner->getName().c_str() << " page reference page: " << i << " page: " << PAGE_REFERENCE_TABLE->page_reference[i].first << " used: " << PAGE_REFERENCE_TABLE->page_reference[i].second << std::endl;
				OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
				tse->setResp(0, PAGE_REFERENCE_TABLE->page_reference[i].first, PAGE_REFERENCE_TABLE->page_reference[i].second);
				to_opal->send(tse);
			}
			else
				break;
		}

		std::map<Address_t, int> pages_to_migrate;
		for(int num_pages_to_migrate=0; num_pages_to_migrate<ev->getSize(); num_pages_to_migrate++)
		{
			int found = 0;
			Address_t page_address;

			for(int i=0; i<100; i++) { // number of checks to get a victim pages
				//std::cerr << Owner->getName().c_str() << " Page table reference index " << i << " address: " << (*PTR)[PTR_REF_POINTER].first << " reference : " << (*PTR)[PTR_REF_POINTER].second
				//		<< " index: " << PTR_REF_POINTER << " (*PTR).size(): " << (*PTR).size() << " PTR_REF_POINTER%(*PTR).size(): " << PTR_REF_POINTER%(*PTR).size() << std::endl;
				if((*PTR)[PTR_REF_POINTER].second == 0 && pages_to_migrate.find((*PTR)[PTR_REF_POINTER].first) == pages_to_migrate.end()) {
					found = 1;
					page_address = (*PTR)[PTR_REF_POINTER].first;
					break;
				}
				else
					(*PTR)[PTR_REF_POINTER].second = 0;

				PTR_REF_POINTER = (PTR_REF_POINTER+1)%(*PTR).size();
			}
			PTR_REF_POINTER = (PTR_REF_POINTER+1)%(*PTR).size();

			while(!found) {
				(*PTR)[PTR_REF_POINTER].second = 0;
				if(pages_to_migrate.find((*PTR)[PTR_REF_POINTER].first) == pages_to_migrate.end()) {
					page_address = (*PTR)[PTR_REF_POINTER].first;
					found = 1;
				}
				PTR_REF_POINTER = (PTR_REF_POINTER+1)%(*PTR).size();
			}

			OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
			tse->setResp(0, page_address, ev->getSize());
			to_opal->send(tse);

			pages_to_migrate[page_address] = 0;
		}
	}
	break;*/

	/*case SST::OpalComponent::EventType::REMAP:
	{
		*shootdown = 1;
		shootdownId = ev->getSize();	// for future case

		Address_t newPaddress = ev->getPaddress();
		Address_t vaddress = ev->getAddress();
		int faultLevel = ev->getFaultLevel();
		//bool invalidate = ev->getInvalidate();

		// update physical address. get the level to update
		// only by core 0
		//std::cerr << Owner->getName().c_str() << " Core ID: " << coreId << " Invalidate address: " << vaddress << " new paddress: " << newPaddress << std::endl;
		if(0 == coreId) {
			if(faultLevel == 4 ) {
				//std::cerr << Owner->getName().c_str() << " Core ID: " << coreId << " Invalidate CR3 address: " << vaddress << " old paddress: " << *CR3 << " new paddress: " << newPaddress << std::endl;
				*CR3 = newPaddress;
			}
			else if(faultLevel == 3) {
				if((*PGD).find(vaddress) != (*PGD).end()) {
					//std::cerr << Owner->getName().c_str() << " Core ID: " << coreId << " Invalidate PGD address: " << vaddress << " old paddress: " << (*PGD)[vaddress] << " new paddress: " << newPaddress << std::endl;
					(*PGD)[vaddress] = newPaddress;
					pgd_addresses_migrated->addData(1);
				}
				else
					output->fatal(CALL_INFO, -1, "MMU: Shootdown invalidation error!! PGD\n");
			}
			else if(faultLevel == 2) {
				if((*PUD).find(vaddress) != (*PUD).end()){
					//std::cerr << Owner->getName().c_str() << " Core ID: " << coreId << " Invalidate PUD address: " << vaddress << " old paddress: " << (*PUD)[vaddress] << " new paddress: " << newPaddress << std::endl;
					(*PUD)[vaddress] = newPaddress;
					pud_addresses_migrated->addData(1);
				}
				else
					output->fatal(CALL_INFO, -1, "MMU: Shootdown invalidation error!! PUD\n");
			}
			else if(faultLevel == 1) {
				if((*PMD).find(vaddress) != (*PMD).end()) {
					//std::cerr << Owner->getName().c_str() << " Core ID: " << coreId << " Invalidate PMD address: " << vaddress << " old paddress: " << (*PMD)[vaddress] << " new paddress: " << newPaddress << std::endl;
					(*PMD)[vaddress] = newPaddress;
					pmd_addresses_migrated->addData(1);
				}
				else
					output->fatal(CALL_INFO, -1, "MMU: Shootdown invalidation error!! PMD\n");
			}
			else if(faultLevel == 0) {
				if((*PTE).find(vaddress) != (*PTE).end()) {
					//std::cerr << Owner->getName().c_str() << " Core ID: " << coreId << " Invalidate PTE address: " << vaddress << " old paddress: " << (*PTE)[vaddress] << " new paddress: " << newPaddress << std::endl;
					(*PTE)[vaddress] = newPaddress;
					pte_addresses_migrated->addData(1);
				}
				else
					output->fatal(CALL_INFO, -1, "MMU: Shootdown invalidation error!! PTE\n");
			}
			else
				output->fatal(CALL_INFO, -1, "MMU: IVALIDATION DANGER!!\n");

		}
		// store the address to be invalidated
		invalid_addrs->push_back(std::make_pair(ev->getAddress(),faultLevel));
		num_pages_migrated = invalid_addrs->size();
		num_of_pages_migrated->addData(1);

	}
	break;

	case SST::OpalComponent::EventType::SHOOTDOWN:
	{
		*shootdown = 1;
		*hasInvalidAddrs = 1;
		tlb_shootdown->addData(1);
		tlb_shootdown_time = *timeStamp;
		//std::cerr << Owner->getName().c_str() << " Core ID: " << coreId << " shootdown initiated: tlb_shootdown_time: " << tlb_shootdown_time << std::endl;
	}
	break;
	*/
	default:
		output->fatal(CALL_INFO, -1, "PTW unknown request from opal\n");
	}

	delete ev;

}

void PageTableWalker::recvResp(SST::Event * ev)
{

	long long int pw_id;
	if(!self_connected)
		pw_id = MEM_REQ[((MemEvent*) ev)->getResponseToID()];
	else
		pw_id = MEM_REQ[((MemEvent*) ev)->getID()];	

	Address_t addr = ((MemEvent*) ev)->getVirtualAddress();

	//std::cerr << Owner->getName().c_str() << " ptw response address: " << ((MemEvent*) ev)->getBaseAddr() << " virt address: " << addr << " level: " << WSR_COUNT[pw_id] << " mmu_id: " << pw_id << std::endl;

	insert_way(addr, find_victim_way(addr, WSR_COUNT[pw_id]), WSR_COUNT[pw_id]);

	WSR_READY[pw_id]=true;

	// Avoiding memory leak by deleting the newly generated dummy requests
	MEM_REQ.erase(((MemEvent*) ev)->getID());
	delete ev;

	if(WSR_COUNT[pw_id]==0)
	{
		ready_by[WID_EV[pw_id]] =  currTime + latency + 2*upper_link_latency;

		ready_by_size[WID_EV[pw_id]] = os_page_size; // FIXME: This hardcoded for now assuming the OS maps virtual pages to 4KB pages only

		//std::cerr << Owner->getName().c_str() << " ptw vaddr: " << ((MemEvent *)WID_EV[pw_id])->getVirtualAddress() << " addr: " << ((MemEvent *)WID_EV[pw_id])->getBaseAddr() << " level: " << WSR_COUNT[pw_id] << std::endl;

		hold_for_mem = 0;
	}
	else
	{
		Address_t dummy_add;
		if(*STU_enabled) {
			dummy_add = rand()%100000000;
			dummy_add += *local_memory_size;
		}
		else
			dummy_add = rand()%10000000;

		// Time to use actual page table addresses if we have page tables
		if(emulate_faults)
		{
			if(WSR_COUNT[pw_id]==4) {
				dummy_add = (*CR3) + ((addr/page_size[3])%512)*8;
				//pgd_accesses->addData(1);
				//if((*CR3) < *local_memory_size) dummy_add = rand()%10000000;
				//else *local_memory_size + rand()%10000000;
				//std::cerr<<"Sending a new PGD request with address1 "<<std::hex<<dummy_add<<" vaddress: " << addr << " CR3 address: " << (*CR3) << " offset: " << ((addr/page_size[3])%512)*8 << " virt address: " << addr <<std::endl;
			}
			else if(WSR_COUNT[pw_id]==3) {
				dummy_add = (*PGD)[(addr/page_size[3])%512] + ((addr/page_size[2])%512)*8;
				/*if(*page_placement && (*PGD)[addr/page_size[3]] < *local_memory_size) {
					//if((*PTR_map).find((*PGD)[addr/page_size[3]]) == (*PTR_map).end())
					//	std::cout<<"Error: page reference is not mapped, vaddress:" << addr/page_size[3] << " physical page: " << (*PGD)[addr/page_size[3]] << std::endl;

					//(*PTR)[(*PTR_map)[(*PGD)[addr/page_size[3]]]].second = 1;
					OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
					tse->setResp(0, (*PGD)[addr/page_size[3]], 1);
					to_opal->send(tse);
					//uint64_t paddr = (*PGD)[addr/page_size[3]];
					std::cerr << Owner->getName().c_str() << " updating page reference to opal page: " << paddr << " address: " << paddr*4096 << " PGD" << std::endl;
				}*/
				//std::cerr<<"Sending a new PUD request with address1 "<<std::hex<<dummy_add<<" vaddress: " << addr << " PUD page: " << (*PGD)[(addr/page_size[3])%512]<<" offset: "<<((addr/page_size[2])%512)*8 << " virt address: " << addr <<std::endl;
				//pud_accesses->addData(1);
			}
			else if(WSR_COUNT[pw_id]==2) {
				dummy_add = (*PUD)[(addr/page_size[2])%(512*512)] + ((addr/page_size[1])%512)*8;
				/*if(*page_placement && (*PUD)[addr/page_size[2]] < *local_memory_size) {
					//if((*PTR_map).find((*PUD)[addr/page_size[2]]) == (*PTR_map).end())
					//	std::cout<<"Error: page reference is not mapped, vaddress:" << addr/page_size[2] << " physical page: " << (*PUD)[addr/page_size[2]] << std::endl;

					//(*PTR)[(*PTR_map)[(*PUD)[addr/page_size[2]]]].second = 1;
					OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
					tse->setResp(0, (*PUD)[addr/page_size[2]], 1);
					to_opal->send(tse);
					//uint64_t paddr = (*PUD)[addr/page_size[2]];
					//std::cerr << Owner->getName().c_str() << " updating page reference to opal page: " << paddr << " address: " << paddr*4096 << " PUD" << std::endl;
				}*/
				//std::cerr<<"Sending a new PMD request with address1 "<<std::hex<<dummy_add<<" vaddress: " << addr<<" PMD page: " << (*PUD)[(addr/page_size[2])%(512*512)]<<" offset: "<<((addr/page_size[1])%512)*8 << " virt address: " << addr <<std::endl;
			//	pmd_accesses->addData(1);
			}
			else if(WSR_COUNT[pw_id]==1) {
				uint64_t offset = (uint64_t)512*512*512;
				dummy_add = (*PMD)[(addr/page_size[1])%offset] + ((addr/page_size[0])%512)*8;
				/*if(*page_placement && (*PMD)[addr/page_size[1]] < *local_memory_size) {
					//if((*PTR_map).find((*PMD)[addr/page_size[1]]) == (*PTR_map).end())
					//	std::cout<<"Error: page reference is not mapped, vaddress:" << addr/page_size[1] << " physical page: " << (*PMD)[addr/page_size[1]] << std::endl;

					//(*PTR)[(*PTR_map)[(*PMD)[addr/page_size[1]]]].second = 1;
					OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
					tse->setResp(0, (*PMD)[addr/page_size[1]], 1);
					to_opal->send(tse);
					//uint64_t paddr = (*PMD)[addr/page_size[1]];
					//std::cerr << Owner->getName().c_str() << " updating page reference to opal page: " << paddr << " address: " << paddr*4096 << " PMD" << std::endl;

				}*/
				//std::cerr<<"Sending a new PTE request with address1 "<<std::hex<<dummy_add<<" vaddress: " << addr<<" PTE page: " << (*PMD)[(addr/page_size[1])%(512*512*512)]<<" offset: "<<((addr/page_size[1])%512)*8 << " virt address: " << addr <<std::endl;
				//pte_accesses->addData(1);
			}
			else
				output->fatal(CALL_INFO, -1, "MMU: PTW DANGER!!\n");
			//dummy_add = page_table_start + (addr/page_size[WSR_COUNT[pw_id]-1])%512;

		}
		Address_t dummy_base_add = dummy_add & ~(line_size - 1);
		if(*STU_enabled)
			dummy_base_add = (dummy_add /64) *64;
		if(WSR_COUNT[pw_id]==4) {
			if(pgd_unique_accesses.find(dummy_base_add) == pgd_unique_accesses.end()) {
				pgd_unique_accesses[dummy_base_add] = 1;
			//	pgd_unique_access->addData(1);
			}
		}
		else if(WSR_COUNT[pw_id]==3) {
			if(pud_unique_accesses.find(dummy_base_add) == pud_unique_accesses.end()) {
				pud_unique_accesses[dummy_base_add] = 1;
			//	pud_unique_access->addData(1);
			}
		}
		else if(WSR_COUNT[pw_id]==2) {
			if(pmd_unique_accesses.find(dummy_base_add) == pmd_unique_accesses.end()) {
				pmd_unique_accesses[dummy_base_add] = 1;
		//		pmd_unique_access->addData(1);
			}
		}
		else if(WSR_COUNT[pw_id]==1) {
			if(pte_unique_accesses.find(dummy_base_add) == pte_unique_accesses.end()) {
				pte_unique_accesses[dummy_base_add] = 1;
	//			pte_unique_access->addData(1);
			}
		}
		else output->fatal(CALL_INFO, -1, "MMU: PTW DANGER!!\n");

		MemEvent *e = new MemEvent(Owner, dummy_add, dummy_base_add, Command::GetS);
		e->setPT();
		//e->setFlag(MemEvent::F_NONCACHEABLE);
		e->setVirtualAddress(addr);
		SST::Event * event = e;
		//std::cerr << Owner->getName().c_str() << " core: " << coreId <<" ptw address: " << e->getAddr() << " base address: " << e->getBaseAddr() <<" vaddress: " << addr<< std::endl;
		//std::cerr << Owner->getName().c_str() << " ptw vaddr: " << ((MemEvent *)WID_EV[pw_id])->getVirtualAddress() << " addr: " << e->getBaseAddr() << " level: " << WSR_COUNT[pw_id] << std::endl;
		WSR_COUNT[pw_id]--;
		MEM_REQ[e->getID()]=pw_id;
		to_mem->send(event);


	}


}


bool PageTableWalker::tick(SST::Cycle_t x)
{


	currTime = x;

	// In case we are currently servicing a page fault, just return until this is dealt with
	if(stall && emulate_faults)
	{

		//std::cout<< Owner->getName().c_str() << " Core: " << coreId << " stalled with stall address: " << stall_addr << std::endl;
		uint64_t offset = (uint64_t)512*512*512*512;
		int release = 0;
		switch(stall_at_levels) {
		case 4:
		{
			if((*PENDING_PAGE_FAULTS_PGD).find((stall_addr/page_size[3])%(512))==(*PENDING_PAGE_FAULTS_PGD).end() &&
				(*PENDING_PAGE_FAULTS_PUD).find((stall_addr/page_size[2])%(512*512))==(*PENDING_PAGE_FAULTS_PUD).end() &&
				(*PENDING_PAGE_FAULTS_PMD).find((stall_addr/page_size[1])%(512*512*512))==(*PENDING_PAGE_FAULTS_PMD).end() &&
				(*PENDING_PAGE_FAULTS_PTE).find((stall_addr/page_size[0])%(offset))==(*PENDING_PAGE_FAULTS_PTE).end())
			{
				release = 1;
			}

		}
			break;
		case 3:
		{
			if((*PENDING_PAGE_FAULTS_PUD).find((stall_addr/page_size[2])%(512*512))==(*PENDING_PAGE_FAULTS_PUD).end() &&
				(*PENDING_PAGE_FAULTS_PMD).find((stall_addr/page_size[1])%(512*512*512))==(*PENDING_PAGE_FAULTS_PMD).end() &&
				(*PENDING_PAGE_FAULTS_PTE).find((stall_addr/page_size[0])%(offset))==(*PENDING_PAGE_FAULTS_PTE).end())
			{
				release = 1;
			}
		}
			break;
		case 2:
		{
			if((*PENDING_PAGE_FAULTS_PMD).find((stall_addr/page_size[1])%(512*512*512))==(*PENDING_PAGE_FAULTS_PMD).end() &&
				(*PENDING_PAGE_FAULTS_PTE).find((stall_addr/page_size[0])%(offset))==(*PENDING_PAGE_FAULTS_PTE).end())
			{
				release = 1;
			}
		}
			break;
		case 1:
		{
			if(stall_at_PGD) {if((*PENDING_PAGE_FAULTS_PGD).find((stall_addr/page_size[3])%(512))==(*PENDING_PAGE_FAULTS_PGD).end()) release = 1;}
			else if(stall_at_PUD) {if((*PENDING_PAGE_FAULTS_PUD).find((stall_addr/page_size[2])%(512*512))==(*PENDING_PAGE_FAULTS_PUD).end()) release = 1;}
			else if(stall_at_PMD) {if((*PENDING_PAGE_FAULTS_PMD).find((stall_addr/page_size[1])%(512*512*512))==(*PENDING_PAGE_FAULTS_PMD).end()) release = 1;}
			else if(stall_at_PTE) {if((*PENDING_PAGE_FAULTS_PTE).find((stall_addr/page_size[0])%(offset))==(*PENDING_PAGE_FAULTS_PTE).end()) release = 1;}
			else output->fatal(CALL_INFO, -1, "MMU: PTW DANGER!!.. stall at level not recognized..\n");
		}
			break;

		default:
			output->fatal(CALL_INFO, -1, "MMU: PTW DANGER!!.. stall at levels not recognized\n");
		}
		if(release) {
			//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " stalled address: " << stall_addr << " released: "<<stall_at_levels<< std::endl;
			stall = false;
			*hold = 0;
		}

		return false;
	}


	// The actual dipatching process... here we take a request and place it in the right queue based on being miss or hit and the number of pending misses
	std::vector<SST::Event *>::iterator st_1,en_1;
	st_1 = not_serviced.begin();
	en_1 = not_serviced.end();

	int dispatched=0;
	for(;st_1!=not_serviced.end() && !(*shootdown) && !(*hold) && (!hold_for_mem); st_1++)
	{
		dispatched++;

		// Checking that we never dispatch more accesses than the port width
		if(dispatched > max_width)
			break;

		SST::Event * ev = *st_1; 
		Address_t addr = ((MemEvent*) ev)->getVirtualAddress();
		if(*STU_enabled)
			addr = ((MemEvent*) ev)->getBaseAddr();
		//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " address "<<addr<< " hold: "<< *hold<<std::endl;
		// A sneak-peak if the access is going to cause a page fault
		if(emulate_faults==1)
		{
			uint64_t offset = (uint64_t)512*512*512*512;
			bool fault = true;
			if((*MAPPED_PAGE_SIZE4KB).find((addr/page_size[0])%offset)!=(*MAPPED_PAGE_SIZE4KB).end() || (*MAPPED_PAGE_SIZE2MB).find(addr/page_size[1])!=(*MAPPED_PAGE_SIZE2MB).end() || (*MAPPED_PAGE_SIZE1GB).find(addr/page_size[2])!=(*MAPPED_PAGE_SIZE1GB).end())
				fault = false;

			if(fault)
			{
				stall = true;
				*hold = 1;
				stall_addr = addr;
				if(to_mem!=NULL) {
				if((*PGD).find((addr/page_size[3])%512) == (*PGD).end()) {
					stall_at_levels = 1;
					stall_at_PGD = 1;
					stall_at_PUD = 0;
					stall_at_PMD = 0;
					stall_at_PTE = 0;
					if((*PENDING_PAGE_FAULTS_PGD).find((addr/page_size[3])%(512))==(*PENDING_PAGE_FAULTS_PGD).end()) {
						(*PENDING_PAGE_FAULTS_PGD)[(addr/page_size[3])%512] = 0;
						(*PENDING_PAGE_FAULTS_PUD)[(addr/page_size[2])%(512*512)] = 0;
						(*PENDING_PAGE_FAULTS_PMD)[(addr/page_size[1])%(512*512*512)] = 0;
						(*PENDING_PAGE_FAULTS_PTE)[(addr/page_size[0])%(offset)] = 0;
						stall_at_levels += 3;
						SambaEvent * tse = new SambaEvent(EventType::PAGE_FAULT);
						//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " Fault at PGD address "<<addr<< " hold: "<< *hold<<std::endl;
						tse->setResp(addr,0,4096);
						s_EventChan->send(tse);
					}
					else {
						//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " waiting at PGD address "<<addr<< " hold: "<< *hold<<std::endl;
						return false;
					}
				}
				else if((*PUD).find((addr/page_size[2])%(512*512)) == (*PUD).end()) {
					stall_at_levels = 1;
					stall_at_PGD = 0;
					stall_at_PUD = 1;
					stall_at_PMD = 0;
					stall_at_PTE = 0;
					if((*PENDING_PAGE_FAULTS_PUD).find((addr/page_size[2])%(512*512))==(*PENDING_PAGE_FAULTS_PUD).end()) {
						(*PENDING_PAGE_FAULTS_PUD)[(addr/page_size[2])%(512*512)] = 0;
						(*PENDING_PAGE_FAULTS_PMD)[(addr/page_size[1])%(512*512*512)] = 0;
						(*PENDING_PAGE_FAULTS_PTE)[(addr/page_size[0])%(offset)] = 0;
						stall_at_levels += 2;
						SambaEvent * tse = new SambaEvent(EventType::PAGE_FAULT);
						//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " Fault at PUD address "<<addr<< " hold: "<< *hold<<std::endl;
						tse->setResp(addr,0,4096);
						s_EventChan->send(tse);
					}
					else {
						//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " waiting at PUD address "<<addr<< " hold: "<< *hold<<std::endl;
						return false;
					}
				}
				else if((*PMD).find((addr/page_size[1])%(512*512*512)) == (*PMD).end()) {
					stall_at_levels = 1;
					stall_at_PGD = 0;
					stall_at_PUD = 0;
					stall_at_PMD = 1;
					stall_at_PTE = 0;
					if((*PENDING_PAGE_FAULTS_PMD).find((addr/page_size[1])%(512*512*512))==(*PENDING_PAGE_FAULTS_PMD).end()) {
						(*PENDING_PAGE_FAULTS_PMD)[(addr/page_size[1])%(512*512*512)] = 0;
						(*PENDING_PAGE_FAULTS_PTE)[(addr/page_size[0])%(offset)] = 0;
						stall_at_levels += 1;
						SambaEvent * tse = new SambaEvent(EventType::PAGE_FAULT);
						//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " Fault at PMD address "<<addr<< " hold: "<< *hold<<std::endl;
						tse->setResp(addr,0,4096);
						s_EventChan->send(tse);
					}
					else {
						//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " waiting at PMD address "<<addr<< " hold: "<< *hold<<std::endl;
						return false;
					}
				}
				else if((*PTE).find((addr/page_size[0])%(offset)) == (*PTE).end()) {
					stall_at_levels = 1;
					stall_at_PGD = 0;
					stall_at_PUD = 0;
					stall_at_PMD = 0;
					stall_at_PTE = 1;
					if((*PENDING_PAGE_FAULTS_PTE).find((addr/page_size[0])%(offset))==(*PENDING_PAGE_FAULTS_PTE).end()) {
						(*PENDING_PAGE_FAULTS_PTE)[(addr/page_size[0])%(offset)] = 0;
						SambaEvent * tse = new SambaEvent(EventType::PAGE_FAULT);
						//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " Fault at PTE address "<<addr<< " hold: "<< *hold<<std::endl;
						tse->setResp(addr,0,4096);
						s_EventChan->send(tse);
					}
					else {
						//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " waiting at PTE address "<<addr<< " hold: "<< *hold<<std::endl;
						return false;
					}
				}
				else
					output->fatal(CALL_INFO, -1, "MMU: PTW DANGER!!.. page faulted not recognized\n");
				}
				else {
					stall_at_levels = 1;
					stall_at_PGD = 0;
					stall_at_PUD = 0;
					stall_at_PMD = 0;
					stall_at_PTE = 1;
					if((*PENDING_PAGE_FAULTS_PTE).find((addr/page_size[0])%(offset))==(*PENDING_PAGE_FAULTS_PTE).end()) {
						(*PENDING_PAGE_FAULTS_PTE)[(addr/page_size[0])%(offset)] = 0;
						SambaEvent * tse = new SambaEvent(EventType::PAGE_FAULT);
						//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " Fault at PTE address "<<addr<< " hold: "<< *hold<<std::endl;
						tse->setResp(addr,0,4096);
						s_EventChan->send(tse);
					}
					else {
						//std::cerr<< Owner->getName().c_str() << " Core: " << coreId << " waiting at PTE address "<<addr<< " hold: "<< *hold<<std::endl;
						return false;
					}
				}
//				(*MAPPED_PAGE_SIZE4KB)[addr/page_size[0]] = 0; // FIXME: Hack to avoid propogating faulting VA through all events, only for initial testing
				return false;
			}

		}

		// Those track if any hit in one of the supported pages' structures
		int hit_id=0;

		// We check the structures in parallel to find if it hits
		int k;
		for(k=0; k < sizes; k++)
			if(check_hit(addr, k))
			{
				hit_id=k;
				break;
			}

		//if(!(*STU_enabled)) {
		//if(to_mem==NULL || !active_PTW) { k=0; hit_id=0; }
		if(to_mem==NULL) { k=0; hit_id=0; }
		//}
		//else {
			//std::cerr << Owner->getName().c_str() << " STU_enabled: " << STU_enabled << " STU_enabled_PTW: " << STU_active_PTW << std::endl;
		//	if(!STU_active_PTW) {
				//std::cerr << Owner->getName().c_str() << " STU_enabled_PTW: " << STU_active_PTW << std::endl;
		//		k=0; hit_id=0;
		//	}
		//}

		// Check if we found the entry in the PTWC of PTEs
		if(k==0)
		{

			update_lru(addr, hit_id);
			hits++;
	//		statPageTableWalkerHits->addData(1);
			if(parallel_mode)
				ready_by[ev] = x;
			else
				ready_by[ev] = x + latency;

			// Tracking the hit request size
			ready_by_size[ev] = os_page_size; //page_size[hit_id]/1024;

			st_1 = not_serviced.erase(st_1);
		}
		else
		{


			// Note that this is a hack to reduce the number of walks needed for large pages, however, in case of full-system, the content of the page table 
			// will tell us that no next level, but since we don't have a full-system status, we will just stop at the priori-known leaf level
			if(os_page_size == 2048)
				k = max(k-1, 1);
			else if(os_page_size == 1024*1024)
				k = max(k-2, 1); 


			if((int) pending_misses.size() < (int) max_outstanding)
			{	
			//	statPageTableWalkerMisses->addData(1);
				misses++;
				pending_misses.push_back(*st_1);
				if(to_mem!=NULL)// && active_PTW && !STU_enabled)
				{

					WID_EV[++mmu_id] = (*st_1);

					Address_t dummy_add;
					if(*STU_enabled) {
						dummy_add = rand()%100000000;
						dummy_add += *local_memory_size;
						//std::cerr << Owner->getName().c_str() << " addr: " << dummy_add << " local memory size: " << *local_memory_size << " emulate faults: " << emulate_faults << std::endl;
					}
					else
						dummy_add = rand()%10000000;

					// Use actual page table base to start the walking if we have real page tables
					if(emulate_faults)
					{
						if(k==4) {
							dummy_add = (*CR3) + ((addr/page_size[3])%512)*8;
							//std::cerr<<"Sending a new PGD request with address "<<std::hex<<dummy_add<<" vaddress: " << addr << " CR3 address: " << (*CR3) << " offset: " << ((addr/page_size[3])%512)*8<<std::endl;
						//	pgd_accesses->addData(1);
							//if(STU_enabled) { requests_for_PGD[(addr/page_size[3])%512].push_back(*st_1); }
						}
						else if(k==3) {
							dummy_add = (*PGD)[(addr/page_size[3])%512] + ((addr/page_size[2])%512)*8;
							/*if(*page_placement && (*PGD)[addr/page_size[3]] < *local_memory_size) {
								//if((*PTR_map).find((*PGD)[addr/page_size[3]]) == (*PTR_map).end())
								//	std::cout<<"Error: page reference is not mapped, vaddress:" << addr/page_size[3] << " physical page: " << (*PGD)[addr/page_size[3]] << std::endl;

								//(*PTR)[(*PTR_map)[(*PGD)[addr/page_size[3]]]].second = 1;
								OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
								tse->setResp(0, (*PGD)[addr/page_size[3]], 1);
								to_opal->send(tse);
								//uint64_t paddr = (*PGD)[addr/page_size[3]];
								//std::cerr << Owner->getName().c_str() << " updating page reference to opal page: " << paddr << " address: " << paddr*4096 << " PGD" << std::endl;
							}*/
							//std::cerr<<"Sending a new PUD request with address "<<std::hex<<dummy_add<<" vaddress: " << addr<<" PUD page: " << (*PGD)[(addr/page_size[3])%512]<<" offset: "<<((addr/page_size[2])%512)*8<<std::endl;
						//	pud_accesses->addData(1);
						}
						else if(k==2) {
							dummy_add = (*PUD)[(addr/page_size[2])%(512*512)] + ((addr/page_size[1])%512)*8;
							/*if(*page_placement && (*PUD)[addr/page_size[2]] < *local_memory_size) {
								//if((*PTR_map).find((*PUD)[addr/page_size[2]]) == (*PTR_map).end())
								//	std::cout<<"Error: page reference is not mapped, vaddress:" << addr/page_size[2] << " physical page: " << (*PUD)[addr/page_size[2]] << std::endl;

								//(*PTR)[(*PTR_map)[(*PUD)[addr/page_size[2]]]].second = 1;
								OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
								tse->setResp(0, (*PUD)[addr/page_size[2]], 1);
								to_opal->send(tse);
								//uint64_t paddr = (*PUD)[addr/page_size[2]];
								//std::cerr << Owner->getName().c_str() << " updating page reference to opal page: " << paddr << " address: " << paddr*4096 << " PUD" << std::endl;
							}*/
							//std::cerr<<"Sending a new PMD request with address "<<std::hex<<dummy_add<<" vaddress: " << addr<<" PMD page: " << (*PUD)[(addr/page_size[2])%(512*512)]<<" offset: "<<((addr/page_size[1])%512)*8<<std::endl;
					//		pmd_accesses->addData(1);
						}
						else if (k==1) {
							uint64_t offset = (uint64_t)512*512*512;
							dummy_add = (*PMD)[(addr/page_size[1])%offset] + ((addr/page_size[0])%512)*8;
							/*if(*page_placement && (*PMD)[addr/page_size[1]] < *local_memory_size) {
								//if((*PTR_map).find((*PMD)[addr/page_size[1]]) == (*PTR_map).end())
								//	std::cout<<"Error: page reference is not mapped, vaddress:" << addr/page_size[1] << " physical page: " << (*PMD)[addr/page_size[1]] << std::endl;

								//(*PTR)[(*PTR_map)[(*PMD)[addr/page_size[1]]]].second = 1;
								OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
								tse->setResp(0, (*PMD)[addr/page_size[1]], 1);
								to_opal->send(tse);
								//uint64_t paddr = (*PMD)[addr/page_size[1]];
								//std::cerr << Owner->getName().c_str() << " updating page reference to opal page: " << paddr << " address: " << paddr*4096 << " PMD" << std::endl;
							}*/
							//std::cerr<<"Sending a new PTE request with address "<<std::hex<<dummy_add<<" vaddress: " << addr<<" PTE page: " << (*PMD)[(addr/page_size[1])%(512*512*512)]<<" offset: "<<((addr/page_size[0])%512)*8<<std::endl;
						//	pte_accesses->addData(1);
						}
						else
							output->fatal(CALL_INFO, -1, "MMU: PTW DANGER!!\n");
					}

					Address_t dummy_base_add = dummy_add & ~(line_size - 1);
					if(*STU_enabled)
						dummy_base_add = (dummy_add /64) *64;
					if(k==4) {
						if(pgd_unique_accesses.find(dummy_base_add) == pgd_unique_accesses.end()) {
							pgd_unique_accesses[dummy_base_add] = 1;
						//	pgd_unique_access->addData(1);
						}
					}
					else if(k==3) {
						if(pud_unique_accesses.find(dummy_base_add) == pud_unique_accesses.end()) {
							pud_unique_accesses[dummy_base_add] = 1;
							//pud_unique_access->addData(1);
						}
					}
					else if(k==2) {
						if(pmd_unique_accesses.find(dummy_base_add) == pmd_unique_accesses.end()) {
							pmd_unique_accesses[dummy_base_add] = 1;
						//	pmd_unique_access->addData(1);
						}
					}
					else if(k==1) {
						if(pte_unique_accesses.find(dummy_base_add) == pte_unique_accesses.end()) {
							pte_unique_accesses[dummy_base_add] = 1;
					//		pte_unique_access->addData(1);
						}
					}
					else output->fatal(CALL_INFO, -1, "MMU: PTW DANGER!!\n");

					MemEvent *e = new MemEvent(Owner, dummy_add, dummy_base_add, Command::GetS);
					e->setPT();
					//e->setFlag(MemEvent::F_NONCACHEABLE);
					SST::Event * ev = e;
					//std::cerr << Owner->getName().c_str() << " core: " << coreId << " ptw address: " << e->getAddr() << " base address: " << e->getBaseAddr()<<" vaddress: " << addr << std::endl;
					//PAGE_REFERENCE_TABLE->updatePageReference();

					WSR_COUNT[mmu_id] = k-1;
					//		WID_EV[mmu_id] = ev;
					//WID_Add[mmu_id] = addr;
					e->setVirtualAddress(addr);
					WSR_READY[mmu_id] = false;					

					// Add it to the tracking structure
					MEM_REQ[e->getID()]=mmu_id;

					//std::cerr << Owner->getName().c_str()  << " ptw vaddr: " << ((MemEvent *)WID_EV[mmu_id])->getVirtualAddress() << " ptw addr: " << dummy_add << " level: " << k<<"("<<WSR_COUNT[mmu_id]<<")"<< " mmu_id: "<<mmu_id<<" virt address: " <<e->getVirtualAddress()<< std::endl;
					hold_for_mem = 1;
					//	std::cerr<<Owner->getName().c_str() << "Sending a new request with address "<<std::hex<<dummy_add<<std::endl;
					// Actually send the event to the cache
					to_mem->send(ev);


					st_1 = not_serviced.erase(st_1);


				}
				else
				{
					//if(STU_enabled && STU_active_PTW) {
						//std::cerr << "stu active ptw" << endl;
					//	ready_by[ev] = x + latency + 2*upper_link_latency + page_walk_latency;  // the upper link latency is substituted for sending the miss request and reciving it, Note this is hard coded for the last-level as memory access walk latency, this ****definitely**** needs to change
					//}
					//else
						ready_by[ev] = x + latency + 2*upper_link_latency;// + page_walk_latency;

					ready_by_size[ev] = os_page_size; // FIXME: This hardcoded for now assuming the OS maps virtual pages to 4KB pages only

					st_1 = not_serviced.erase(st_1);
				}

			}

		}	    


		if(st_1 == not_serviced.end())
			break;
	}


	std::map<SST::Event *, SST::Cycle_t>::iterator st, en;
	st = ready_by.begin();
	en = ready_by.end();

	while(st!=en)
	{

		bool deleted=false;
		if(st->second <= x)
		{

			Address_t addr = ((MemEvent*) st->first)->getVirtualAddress();
			if(*STU_enabled)
				addr = ((MemEvent*) st->first)->getBaseAddr();

			// Double checking that we actually still don't have it inserted
			//std::cout<<"The address is"<<addr<<std::endl;
			if(!check_hit(addr, 0))
			{
				insert_way(addr, find_victim_way(addr, 0), 0);
				update_lru(addr, 0);
			}
			else
				update_lru(addr, 0);

			//std::cerr << Owner->getName().c_str() << " PTW serviced request: " << addr << std::endl;
			service_back->push_back(st->first);


			if(emulate_faults)
				if((*PTE).find(addr/4096)==(*PTE).end())
                        {
					std::cout << "******* Major issue is in Page Table Walker **** " << std::endl;
					std::cout << "The address is "<< hex << addr << " (" << addr / 4096 << ")" << std::endl;
                        }
			(*service_back_size)[st->first]=ready_by_size[st->first];


			ready_by_size.erase(st->first);




			deleted = true;


			// Deleting it from pending requests
			std::vector<SST::Event *>::iterator st2, en2;
			st2 = pending_misses.begin();
			en2 = pending_misses.end();


			while(st2!=en2)
			{
				if(*st2 == st->first)
				{
					pending_misses.erase(st2);
					break;
				}
				st2++;
			}

			ready_by.erase(st);

		}
		if(deleted)
			st=ready_by.begin();
		else
			st++;

	}



	return false;
}


void PageTableWalker::initaitePageMigration(Address_t vaddress, Address_t paddress)
{

	/*
	 * Check if any shootdon request with same address is pending.
	 * If No, send a TLB shootdown request to Opal and stall
	 * If Yes, Just stall.
	 */
	//std::cout << Owner->getName().c_str() << " Core ID: " << coreId << " sending TLB shootdown with address: " << std::hex << vaddress << " new paddress: " << paddress << std::endl;
	stall_addr = vaddress;
	/*if((*PENDING_SHOOTDOWN_EVENTS).find(vaddress/page_size[0]) == (*PENDING_SHOOTDOWN_EVENTS).end()) {
		(*PENDING_SHOOTDOWN_EVENTS)[vaddress/page_size[0]] = 0;
		(*PENDING_PAGE_FAULTS)[vaddress/page_size[0]] = 0;		//add to pending page faults list
		(*MAPPED_PAGE_SIZE4KB).erase(vaddress/page_size[0]); 	//unmap the page
		SambaEvent * tse = new SambaEvent(EventType::SHOOTDOWN);
		tse->setResp(vaddress/page_size[0],paddress,4096);
		s_EventChan->send(tse);
	}*/

}


/*void PageTableWalker::updatePTR(Address_t paddr)
{
	OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
	tse->setResp(0, paddr, 1);
	to_opal->send(tse);
	//std::cerr << Owner->getName().c_str() << " updating page reference to opal page: " << paddr << " address: " << paddr*4096 << std::endl;
}*/


void PageTableWalker::insert_way(Address_t vaddr, int way, int struct_id)
{


	int set=abs_int_Samba((vaddr/page_size[struct_id])%sets[struct_id]);
	tags[struct_id][set][way]=vaddr/page_size[struct_id];
	valid[struct_id][set][way]=true;

}


// Does the translation and updating the statistics of miss/hit
Address_t PageTableWalker::translate(Address_t vadd)
{
	return 1;

}


// Invalidate TLB entries
void PageTableWalker::invalidate(Address_t vadd, int id)
{

	int set= abs_int_Samba((vadd*page_size[0]/page_size[id])%sets[id]);
	for(int i=0; i<assoc[id]; i++) {
		if(tags[id][set][i]==vadd*page_size[0]/page_size[id] && valid[id][set][i]) {
			valid[id][set][i] = false;
			break;
		}
	}
}


// done with shootdown, send shootdown ack
void PageTableWalker::sendShootdownAck(int sd_delay, int page_swapping_delay)
{
	//std::cerr << Owner->getName().c_str() << " core: " << coreId << " shootdown ack" << std::endl;
	//std::cerr << Owner->getName().c_str() << " total delay: " << delay + (num_pages_migrated)*1000 << " shootdown delay: " << delay << " page migration delay: "
	//		<< (num_pages_migrated)*page_swapping_delay << " num_pages_migrated: " << num_pages_migrated << std::endl;
	s_EventChan->send(sd_delay + (num_pages_migrated)*page_swapping_delay, new SambaEvent(SambaComponent::EventType::SDACK));
	//std::cerr << Owner->getName().c_str() << " pending delay: " << delay << std::endl;
}



// Find if it exists
bool PageTableWalker::check_hit(Address_t vadd, int struct_id)
{

	int set= abs_int_Samba((vadd/page_size[struct_id])%sets[struct_id]);

	for(int i=0; i<assoc[struct_id];i++)
		if(tags[struct_id][set][i]==vadd/page_size[struct_id] && valid[struct_id][set][i])
			return true;

	return false;
}

// To insert the translaiton
int PageTableWalker::find_victim_way(Address_t vadd, int struct_id)
{

	int set= abs_int_Samba((vadd/page_size[struct_id])%sets[struct_id]);

	for(int i=0; i<assoc[struct_id]; i++)
		if(lru[struct_id][set][i]==(assoc[struct_id]-1))
			return i;


	return 0;
}

// This function updates the LRU policy for a given address
void PageTableWalker::update_lru(Address_t vaddr, int struct_id)
{

	int lru_place=assoc[struct_id]-1;

	int set= abs_int_Samba((vaddr/page_size[struct_id])%sets[struct_id]);
	for(int i=0; i<assoc[struct_id];i++)
		if(tags[struct_id][set][i]==vaddr/page_size[struct_id])
		{
			lru_place = lru[struct_id][set][i];
			break;
		}
	for(int i=0; i<assoc[struct_id];i++)
	{
		if(lru[struct_id][set][i]==lru_place)
			lru[struct_id][set][i]=0;
		else if(lru[struct_id][set][i]<lru_place)
			lru[struct_id][set][i]++;
	}


}




