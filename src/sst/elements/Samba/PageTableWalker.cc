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

/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */

#include <sst_config.h>
#include "PageTableWalker.h"
#include <sst/core/link.h>
#include "Samba_Event.h"
#include<iostream>

using namespace SST::SambaComponent;
using namespace SST::MemHierarchy;
using namespace SST;



// This is and ID used to merely identify page walking requests
long long int mmu_id=0;


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


PageTableWalker::PageTableWalker(ComponentId_t id, int Page_size, int Assoc, PageTableWalker * Next_level, int Size) : ComponentExtension(id)
{


}


PageTableWalker::PageTableWalker(ComponentId_t id, int tlb_id, PageTableWalker * Next_level, int level, SST::Params& params) : ComponentExtension(id)
{

	output = new SST::Output("SambaComponent[@f:@l:@p] ", 1, 0, SST::Output::STDOUT);

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

	emulate_faults  = ((uint32_t) params.find<uint32_t>("emulate_faults", 0));

	ptw_confined  = ((uint32_t) params.find<uint32_t>("ptw_confined", 0));

	// For now, we assume a typicaly x86-64 system with 4 levels of page table
	sizes = 4;

	parallel_mode = ((uint32_t) params.find<uint32_t>("parallel_mode_L"+LEVEL, 0));

	upper_link_latency = ((uint32_t) params.find<uint32_t>("upper_link_L"+LEVEL, 0));


	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "Core%d_PTWC", tlb_id);


	// The stats that will appear, not that these stats are going to be part of the Samba unit
	statPageTableWalkerHits = registerStatistic<uint64_t>( "tlb_hits", subID);
	statPageTableWalkerMisses = registerStatistic<uint64_t>( "tlb_misses", subID );

	max_width = ((uint32_t) params.find<uint32_t>("max_width_PTWC", 4));

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
				tags[id][i][j]=-1;
				valid[id][i][j]=false;
				lru[id][i][j]=j;
			}
		}

	}



}

// Handling internal events that are sent by the Page Table Walker
void PageTableWalker::handleEvent( SST::Event* e )
{


	// For each page fault,
	// 1 -- check if CR3 for VA exist, if not
	// Send request to page fault handler and save state of current fault at 3
	// one recvPageFaultResp is called due to response from page fault handler, recvPageFaultResp will create an event that will trigger HandleEvent
	// If HandleEvent comes and state is 3, it again changes state to 2 and send a request to page fault handler...
	//
	// Finally, one a physical page is assigned, the stall is set to false, and *hold is reset



	//// ******** Important, for each level, we will update MAPPED_PAGE_SIZE** and PGD/PMD/PUD/PTE so the actual physical address is used later
	SambaEvent * temp_ptr =  dynamic_cast<SambaComponent::SambaEvent*> (e);

	if(temp_ptr==nullptr)
		std::cout<<" Error in Casting to SambaEvent "<<std::endl;

	if(temp_ptr->getType() == EventType::PAGE_FAULT)
	{

		// Send request to page fault handler starting from the first unmapped level (L4/CR3 if first fault in system)

		if(!ptw_confined)
		{
			//if((*CR3) == -1)
			if(!(*cr3_init))
				fault_level = 4;
			else if((*PGD).find(temp_ptr->getAddress()/page_size[3]) == (*PGD).end())
				fault_level = 3;
			else if((*PUD).find(temp_ptr->getAddress()/page_size[2]) == (*PUD).end())
				fault_level = 2;
			else if((*PMD).find(temp_ptr->getAddress()/page_size[1]) == (*PMD).end())
				fault_level = 1;
			else if((*PTE).find(temp_ptr->getAddress()/page_size[0]) == (*PTE).end())
				fault_level = 0;
			else
				output->fatal(CALL_INFO, -1, "MMU: DANGER!!\n");
		}
		else
		{
			uint64_t offset = (uint64_t)512*512*512*512;
			if(!(*cr3_init)) fault_level = 4;
			else if((*PGD).find((temp_ptr->getAddress()/page_size[3])%512) == (*PGD).end()) fault_level = 3;
			else if((*PUD).find((temp_ptr->getAddress()/page_size[2])%(512*512)) == (*PUD).end()) fault_level = 2;
			else if((*PMD).find((temp_ptr->getAddress()/page_size[1])%(512*512*512)) == (*PMD).end()) fault_level = 1;
			else if((*PTE).find((temp_ptr->getAddress()/page_size[0])%offset) == (*PTE).end()) fault_level = 0;
	 		else output->fatal(CALL_INFO, -1, "MMU: DANGER!!\n");
		}

		if(!(*cr3_init)) {
			*cr3_init = 1;
			pageFaultHandler->allocatePage(coreId,fault_level,stall_addr,4096);
		}else
			pageFaultHandler->allocatePage(coreId,fault_level,stall_addr/page_size[fault_level],4096);

	}
	else if(temp_ptr->getType() == EventType::PAGE_FAULT_RESPONSE)
	{

		// If page fault handler response and it is the lst, i.e., PTE, then just do the following:
		// (1) update page table and maps of mapped pages -- (2) stall = false, *hold=0
		// Otherwise
		// Update the page tables to reflect new page table entries/tables, then issue a new page fault handler request to build next level

		// For now, just assume only the page will be mappe and requested from page fault handler
		if(fault_level == 4)
		{
			// We are building the first page in the page table!
			//std::cout << getName().c_str() << " Core: " << coreId << " CR3 address: " << std::hex << temp_ptr->getPaddress() << std::endl;
			*cr3_init = 1;
			(*CR3) = temp_ptr->getPaddress();
			fault_level--;
			pageFaultHandler->allocatePage(coreId,fault_level,stall_addr/page_size[fault_level],4096);
		}
		else if(fault_level == 3)
		{
			if(!ptw_confined)
				(*PGD)[stall_addr/page_size[3]] = temp_ptr->getPaddress();
			else
			{
				if((*PGD).find((stall_addr/page_size[3])%512) != (*PGD).end())
					output->fatal(CALL_INFO, -1, "MMU: PTW DANGER.. same PGD!!\n");
				(*PGD)[(stall_addr/page_size[3])%512] = temp_ptr->getPaddress();
				(*PENDING_PAGE_FAULTS_PGD).erase((stall_addr/page_size[3])%(512));
			}
			fault_level--;
			pageFaultHandler->allocatePage(coreId,fault_level,stall_addr/page_size[fault_level],4096);

		}
		else if(fault_level == 2)
		{
			if(!ptw_confined)
				(*PUD)[stall_addr/page_size[2]] = temp_ptr->getPaddress();
			else
			{
				if((*PUD).find((stall_addr/page_size[2])%(512*512)) != (*PUD).end())
					output->fatal(CALL_INFO, -1, "MMU: PTW DANGER.. same PUD!!\n");
				(*PUD)[(stall_addr/page_size[2])%(512*512)] = temp_ptr->getPaddress();
				(*PENDING_PAGE_FAULTS_PUD).erase((stall_addr/page_size[2])%(512*512));
			}
			//if(temp_ptr->getSize() == page_size[2]) {
			//	(*MAPPED_PAGE_SIZE1GB)[temp_ptr->getAddress()/page_size[2]] = 0;
			//	fault_level = 0;
			//	stall = false;
			//	*hold = 0;

			//} else {
				fault_level--;
				pageFaultHandler->allocatePage(coreId,fault_level,stall_addr/page_size[fault_level],4096);

			//}

		}

		else if(fault_level == 1)
		{
			if(!ptw_confined)
				(*PMD)[stall_addr/page_size[1]] = temp_ptr->getPaddress();
			else
			{
				uint64_t offset = 512*512*512;
				if((*PMD).find((stall_addr/page_size[1])%offset) != (*PMD).end())
					output->fatal(CALL_INFO, -1, "MMU: PTW DANGER.. same PMD!!\n");
				(*PMD)[(stall_addr/page_size[1])%offset] = temp_ptr->getPaddress();
				(*PENDING_PAGE_FAULTS_PMD).erase((stall_addr/page_size[1])%offset);
			}
			//if(temp_ptr->getSize() == page_size[1]) {
			//	(*MAPPED_PAGE_SIZE2MB)[temp_ptr->getAddress()/page_size[1]] = 0;
			//	fault_level = 0;
			//	stall = false;
			//	*hold = 0;

			//} else {
				fault_level--;
				pageFaultHandler->allocatePage(coreId,fault_level,stall_addr/page_size[fault_level],4096);

			//}

		}
		else if(fault_level == 0)
		{
			if(!ptw_confined)
				(*PTE)[stall_addr/page_size[0]] = temp_ptr->getPaddress();
			else
			{
				uint64_t offset = (uint64_t)512*512*512*512;
				if((*PTE).find((stall_addr/page_size[0])%offset) != (*PTE).end())
					output->fatal(CALL_INFO, -1, "MMU: PTW DANGER.. same PTE!!\n");
				(*PTE)[(stall_addr/page_size[0])%offset] = temp_ptr->getPaddress();
			}
			SambaEvent * tse = new SambaEvent(EventType::PAGE_FAULT_SERVED);
			s_EventChan->send(tse);
		}


	}
	else if(temp_ptr->getType() == EventType::PAGE_FAULT_SERVED)
	{
		if(!ptw_confined)
		{
			(*MAPPED_PAGE_SIZE4KB)[stall_addr/page_size[0]] = 0;
			(*PENDING_PAGE_FAULTS).erase(stall_addr/page_size[0]);
		}
		else
		{
			uint64_t offset = (uint64_t)512*512*512*512;
			(*MAPPED_PAGE_SIZE4KB)[(stall_addr/page_size[0])%offset] = 0;
			(*PENDING_PAGE_FAULTS_PTE).erase((stall_addr/page_size[0])%offset);
		}
	}
	delete temp_ptr;

}




bool PageTableWalker::recvPageFaultResp(PageFaultHandler::PageFaultHandlerPacket pkt)
{

	switch(pkt.action) {
	case PageFaultHandler::PageFaultHandlerAction::RESPONSE:
	{
		SambaEvent * tse = new SambaEvent(EventType::PAGE_FAULT_RESPONSE);
		tse->setResp(pkt.vAddress, pkt.pAddress,pkt.size);
		s_EventChan->send(10, tse);
	}
	break;

	default:
		output->fatal(CALL_INFO, -1, "PTW unknown request from page fault handler\n");
	}

	return true;
}

void PageTableWalker::recvResp(SST::Event * event)
{


	MemEvent * ev = static_cast<MemEvent*>(event);


	long long int pw_id;
	if(!self_connected)
		pw_id = MEM_REQ[ev->getResponseToID()];
	else
		pw_id = MEM_REQ[ev->getID()];

	insert_way(WID_Add[pw_id], find_victim_way(WID_Add[pw_id], WSR_COUNT[pw_id]), WSR_COUNT[pw_id]);

	Address_t addr = WID_Add[pw_id];

	WSR_READY[pw_id]=true;

	// Avoiding memory leak by deleting the newly generated dummy requests
	MEM_REQ.erase(ev->getID());
	delete ev;

	if(WSR_COUNT[pw_id]==0)
	{
		ready_by[WID_EV[pw_id]] =  currTime + latency + 2*upper_link_latency;

		ready_by_size[WID_EV[pw_id]] = os_page_size; // FIXME: This hardcoded for now assuming the OS maps virtual pages to 4KB pages only
	}
	else
	{


		Address_t dummy_add = rand()%10000000;

		// Time to use actual page table addresses if we have page tables
		if(emulate_faults)
		{
			if(!ptw_confined)
			{
				Address_t page_table_start = 0;
				if(WSR_COUNT[pw_id]==4)
					page_table_start = (*PGD)[addr/page_size[3]];
				else if(WSR_COUNT[pw_id]==3)
					page_table_start = (*PUD) [addr/page_size[2]];
				else if(WSR_COUNT[pw_id]==2)
					page_table_start = (*PMD) [addr/page_size[1]];
				else if (WSR_COUNT[pw_id] == 1)
					page_table_start = (*PTE) [addr/page_size[0]];

				dummy_add = page_table_start + (addr/page_size[WSR_COUNT[pw_id]-1])%512;
			}
			else
			{
				if(WSR_COUNT[pw_id]==4) {
					dummy_add = (*CR3) + ((addr/page_size[3])%512)*8;
				}
				else if(WSR_COUNT[pw_id]==3) {
					dummy_add = (*PGD)[(addr/page_size[3])%512] + ((addr/page_size[2])%512)*8;
				}
				else if(WSR_COUNT[pw_id]==2) {
					dummy_add = (*PUD)[(addr/page_size[2])%(512*512)] + ((addr/page_size[1])%512)*8;}
				else if(WSR_COUNT[pw_id]==1) {
					uint64_t offset = (uint64_t)512*512*512;
					dummy_add = (*PMD)[(addr/page_size[1])%offset] + ((addr/page_size[0])%512)*8;
				}
				else
					output->fatal(CALL_INFO, -1, "MMU: PTW DANGER!!\n");
			}
		}
		Address_t dummy_base_add = dummy_add & ~(line_size - 1);
		MemEvent *e = new MemEvent(getName(), dummy_add, dummy_base_add, Command::GetS);
		e->setVirtualAddress(addr);

		WSR_COUNT[pw_id]--;
		MEM_REQ[e->getID()]=pw_id;
		to_mem->send(e);


	}


}


bool PageTableWalker::tick(SST::Cycle_t x)
{


	currTime = x;

	// In case we are currently servicing a page fault, just return until this is dealt with
	if(stall && emulate_faults)
	{

		if(!ptw_confined)
		{
			//std::cout<< getName().c_str() << " Core: " << coreId << " stalled with stall address: " << stall_addr << std::endl;
			if((*PENDING_PAGE_FAULTS).find(stall_addr/page_size[0])==(*PENDING_PAGE_FAULTS).end()) {
				stall = false;
				*hold = 0;
			}
		}
		else
		{
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
		}
		return false;
	}


	// The actual dipatching process... here we take a request and place it in the right queue based on being miss or hit and the number of pending misses
	std::vector<MemHierarchy::MemEventBase *>::iterator st_1,en_1;
	st_1 = not_serviced.begin();
	en_1 = not_serviced.end();

	int dispatched=0;
	for(;st_1!=not_serviced.end() && !(*shootdown) && !(*hold); st_1++)
	{
		dispatched++;

		// Checking that we never dispatch more accesses than the port width
		if(dispatched > max_width)
			break;

                MemHierarchy::MemEventBase* ev = *st_1;
		Address_t addr = ((MemEvent*) ev)->getVirtualAddress();

		// A sneak-peak if the access is going to cause a page fault
		if(emulate_faults==1)
		{

			bool fault = true;
			if(!ptw_confined)
			{
				if((*MAPPED_PAGE_SIZE4KB).find(addr/page_size[0])!=(*MAPPED_PAGE_SIZE4KB).end() || (*MAPPED_PAGE_SIZE2MB).find(addr/page_size[1])!=(*MAPPED_PAGE_SIZE2MB).end() || (*MAPPED_PAGE_SIZE1GB).find(addr/page_size[2])!=(*MAPPED_PAGE_SIZE1GB).end())
					fault = false;

				if(fault)
				{
					stall_addr = addr;
					if((*PENDING_PAGE_FAULTS).find(addr/page_size[0])==(*PENDING_PAGE_FAULTS).end()) {
						(*PENDING_PAGE_FAULTS)[addr/page_size[0]] = 0;
						SambaEvent * tse = new SambaEvent(EventType::PAGE_FAULT);
						//std::cout<< getName().c_str() << " Core id: " << coreId << " Fault at address "<<addr<<std::endl;
						tse->setResp(addr,0,4096);
						s_EventChan->send(10, tse);

					}

					stall = true;
					*hold = 1;
	//				(*MAPPED_PAGE_SIZE4KB)[addr/page_size[0]] = 0; // FIXME: Hack to avoid propogating faulting VA through all events, only for initial testing
					return false;
				}
			}
			else
			{
				uint64_t offset = (uint64_t)512*512*512*512;
				if((*MAPPED_PAGE_SIZE4KB).find((addr/page_size[0])%offset)!=(*MAPPED_PAGE_SIZE4KB).end() || (*MAPPED_PAGE_SIZE2MB).find((addr/page_size[1])%(512*512*512))!=(*MAPPED_PAGE_SIZE2MB).end() || (*MAPPED_PAGE_SIZE1GB).find((addr/page_size[2])%(512*512))!=(*MAPPED_PAGE_SIZE1GB).end())
 					fault = false;

	 			if(fault)
	 			{
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
							tse->setResp(addr,0,4096);
							s_EventChan->send(tse);
						}
						else {
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
							tse->setResp(addr,0,4096);
							s_EventChan->send(tse);
						}
						else {
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
							tse->setResp(addr,0,4096);
							s_EventChan->send(tse);
						}
						else {
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
							tse->setResp(addr,0,4096);
							s_EventChan->send(tse);
						}
						else {
							return false;
						}
					}
					else {
						return false;
					}
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
							tse->setResp(addr,0,4096);
							s_EventChan->send(tse);
						}
						else {
							return false;
						}
					}
	 				return false;
	 			}
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


		// Check if we found the entry in the PTWC of PTEs
		if(k==0)
		{

			update_lru(addr, hit_id);
			hits++;
			statPageTableWalkerHits->addData(1);
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
				statPageTableWalkerMisses->addData(1);
				misses++;
				pending_misses.push_back(*st_1);
				if(to_mem!=nullptr)
				{

					WID_EV[++mmu_id] = (*st_1);

					Address_t dummy_add = rand()%10000000;

					// Use actual page table base to start the walking if we have real page tables
					if(emulate_faults)
					{
						if(!ptw_confined)
							dummy_add = (*CR3) + (addr/page_size[2])%512;
						else
						{
							if(k==4) {
								dummy_add = (*CR3) + ((addr/page_size[3])%512)*8;
							}
							else if(k==3) {
								dummy_add = (*PGD)[(addr/page_size[3])%512] + ((addr/page_size[2])%512)*8;
							}
							else if(k==2) {
								dummy_add = (*PUD)[(addr/page_size[2])%(512*512)] + ((addr/page_size[1])%512)*8;
							}
							else if (k==1) {
								uint64_t offset = (uint64_t)512*512*512;
								dummy_add = (*PMD)[(addr/page_size[1])%offset] + ((addr/page_size[0])%512)*8;
							}
							else
								output->fatal(CALL_INFO, -1, "MMU: PTW DANGER!!\n");
						}
					}

					Address_t dummy_base_add = dummy_add & ~(line_size - 1);
					MemEvent *e = new MemEvent(getName(), dummy_add, dummy_base_add, Command::GetS);




					WSR_COUNT[mmu_id] = k-1;
					//		WID_EV[mmu_id] = e;
					WID_Add[mmu_id] = addr;
					e->setVirtualAddress(addr);
					WSR_READY[mmu_id] = false;

					// Add it to the tracking structure
					MEM_REQ[e->getID()]=mmu_id;

					//					std::cout<<"Sending a new request with address "<<std::hex<<dummy_add<<std::endl;
					// Actually send the event to the cache
					to_mem->send(e);


					st_1 = not_serviced.erase(st_1);


				}
				else
				{



					ready_by[ev] = x + latency + 2*upper_link_latency + page_walk_latency;  // the upper link latency is substituted for sending the miss request and reciving it, Note this is hard coded for the last-level as memory access walk latency, this ****definitely**** needs to change

					ready_by_size[ev] = os_page_size; // FIXME: This hardcoded for now assuming the OS maps virtual pages to 4KB pages only

					st_1 = not_serviced.erase(st_1);
				}

			}

		}


		if(st_1 == not_serviced.end())
			break;
	}


	std::map<MemHierarchy::MemEventBase *, SST::Cycle_t, MemEventPtrCompare>::iterator st, en;
	st = ready_by.begin();
	en = ready_by.end();

	while(st!=en)
	{

		bool deleted=false;
		if(st->second <= x)
		{


			Address_t addr = ((MemEvent*) st->first)->getVirtualAddress();

			// Double checking that we actually still don't have it inserted
			//std::cout<<"The address is"<<addr<<std::endl;
			if(!check_hit(addr, 0))
			{
				insert_way(addr, find_victim_way(addr, 0), 0);
				update_lru(addr, 0);
			}
			else
				update_lru(addr, 0);


			service_back->push_back(st->first);


			if(emulate_faults)
			{
				if(!ptw_confined)
				{
					if((*PTE).find(addr/4096)==(*PTE).end())
                	{
						std::cout << "******* Major issue is in Page Table Walker **** " << std::endl;
						std::cout << "The address is "<< hex << addr << " (" << addr / 4096 << ")" << std::endl;
                    }
                }
                else
                {
                	uint64_t offset = (uint64_t)512*512*512*512;
					if((*PTE).find((addr/4096)%offset)==(*PTE).end())
                    {
 						std::cout << "******* Major issue is in Page Table Walker **** " << std::endl;
 						std::cout << "The address is "<< hex << addr << " (" << addr / 4096 << ")" << std::endl;
					}
                }
            }

			(*service_back_size)[st->first]=ready_by_size[st->first];


			ready_by_size.erase(st->first);




			deleted = true;


			// Deleting it from pending requests
			std::vector<MemHierarchy::MemEventBase *>::iterator st2, en2;
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
	 * If No, send a TLB shootdown request to page fault handler and stall
	 * If Yes, Just stall.
	 */
	//std::cout << getName().c_str() << " Core ID: " << coreId << " sending TLB shootdown with address: " << std::hex << vaddress << " new paddress: " << paddress << std::endl;
	stall_addr = vaddress;
	/*
	if((*PENDING_SHOOTDOWN_EVENTS).find(vaddress/page_size[0]) == (*PENDING_SHOOTDOWN_EVENTS).end()) {
		(*PENDING_SHOOTDOWN_EVENTS)[vaddress/page_size[0]] = 0;
		(*PENDING_PAGE_FAULTS)[vaddress/page_size[0]] = 0;		//add to pending page faults list
		(*MAPPED_PAGE_SIZE4KB).erase(vaddress/page_size[0]); 	//unmap the page
		SambaEvent * tse = new SambaEvent(EventType::SHOOTDOWN);
		tse->setResp(vaddress/page_size[0],paddress,4096);
		s_EventChan->send(10, tse);
	}

	//std::cout << getName().c_str() << " Core ID: " << coreId << " stall address " << std::hex << stall_addr << " vaddress index "<< vaddress/page_size[0] << std::endl;
	*/
}

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

	for(int id=0; id<sizes; id++)
	{
		int set= abs_int_Samba((vadd*page_size[0]/page_size[id])%sets[id]);
		for(int i=0; i<assoc[id]; i++) {
			if(tags[id][set][i]==vadd*page_size[0]/page_size[id] && valid[id][set][i]) {
				valid[id][set][i] = false;
				break;
			}
		}
	}

}


// done with shootdown, send shootdown ack
void PageTableWalker::sendShootdownAck(int sd_delay, int page_swapping_delay)
{
	s_EventChan->send(sd_delay + (num_pages_migrated)*page_swapping_delay, new SambaEvent(SambaComponent::EventType::SDACK));
}



// Find if it exists
bool PageTableWalker::check_hit(Address_t vadd, int struct_id)
{


	int set= abs_int_Samba((vadd/page_size[struct_id])%sets[struct_id]);

	for(int i=0; i<assoc[struct_id];i++)
		if(tags[struct_id][set][i]==vadd/page_size[struct_id])
			return valid[struct_id][set][i];

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


