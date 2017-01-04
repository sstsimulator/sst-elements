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
#include "PageTableWalker.h"
#include <sst/core/link.h>

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


PageTableWalker::PageTableWalker(int Page_size, int Assoc, PageTableWalker * Next_level, int Size)
{


}


PageTableWalker::PageTableWalker(int tlb_id, PageTableWalker * Next_level, int level, SST::Component * owner, SST::Params& params)
{


	std::string LEVEL = std::to_string(level);

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");



	self_connected = ((uint32_t) params.find<uint32_t>("self_connected", 0));

	page_walk_latency = ((uint32_t) params.find<uint32_t>("page_walk_latency", 50));

	max_outstanding = ((uint32_t) params.find<uint32_t>("max_outstanding_PTWC", 4));

	latency = ((uint32_t) params.find<uint32_t>("latency_PTWC", 1));

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
	page_size = new long long int[sizes];
	sets = new int[sizes];
	tags = new long long int**[sizes];
	lru = new int **[sizes];


	// page table offsets
	page_size[0] = 1024*4;
	page_size[1] = 512*1024*4;
	page_size[2] = (long long int) 512*512*1024*4;
	page_size[3] = (long long int) 512*512*512*1024*4;

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

		tags[id] = new long long int*[sets[id]];

		lru[id] = new int*[sets[id]];

		for(int i=0; i < sets[id]; i++)
		{
			tags[id][i]=new long long int[assoc[id]];
			lru[id][i]=new int[assoc[id]];
			for(int j=0; j<assoc[id];j++)
			{
				tags[id][i][j]=-1;
				lru[id][i][j]=j;
			}
		}

	}



}



void PageTableWalker::recvResp(SST::Event * event)
{


	SST::Event * ev = event;
	uint64_t addr = ((MemEvent*) ev)->getVirtualAddress();


	long long int pw_id;
	if(!self_connected)
		pw_id = MEM_REQ[((MemEvent*) ev)->getResponseToID()];
	else
		pw_id = MEM_REQ[((MemEvent*) ev)->getID()];	

	insert_way(WID_Add[pw_id], find_victim_way(WID_Add[pw_id], WSR_COUNT[pw_id]), WSR_COUNT[pw_id]);


	WSR_READY[pw_id]=true;

	// Avoiding memory leak by deleting the newly generated dummy requests
	MEM_REQ.erase(((MemEvent*) ev)->getID());
	delete ev;

	if(WSR_COUNT[pw_id]==0)
	{
		ready_by[WID_EV[pw_id]] =  currTime + latency + 2*upper_link_latency;

		ready_by_size[WID_EV[pw_id]] = os_page_size; // FIXME: This hardcoded for now assuming the OS maps virtual pages to 4KB pages only
	}
	else
	{


		long long int dummy_add = rand()%10000000;
		MemEvent *e = new MemEvent(Owner, dummy_add, dummy_add, GetS);
		SST::Event * ev = e;

		WSR_COUNT[pw_id]--;
		MEM_REQ[e->getID()]=pw_id;
		to_mem->send(ev);


	}


}


bool PageTableWalker::tick(SST::Cycle_t x)
{


	currTime = x;
	// The actual dipatching process... here we take a request and place it in the right queue based on being miss or hit and the number of pending misses
	std::vector<SST::Event *>::iterator st_1,en_1;
	st_1 = not_serviced.begin();
	en_1 = not_serviced.end();

	int dispatched=0;
	for(;st_1!=not_serviced.end(); st_1++)
	{
		dispatched++;

		// Checking that we never dispatch more accesses than the port width
		if(dispatched > max_width)
			break;

		SST::Event * ev = *st_1; 
		uint64_t addr = ((MemEvent*) ev)->getVirtualAddress();


		// Those track if any hit in one of the supported pages' structures
		bool hit=false;
		int hit_id=0;

		// We check the structures in parallel to find if it hits
		int k;
		for(k=0; k < sizes; k++)
			if(check_hit(addr, k))
			{
				hit=true;
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
			ready_by_size[ev] = page_size[hit_id]/1024;

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


			if(pending_misses.size() < max_outstanding)
			{	
				statPageTableWalkerMisses->addData(1);
				misses++;
				pending_misses.push_back(*st_1);
				if(to_mem!=NULL)
				{

					WID_EV[++mmu_id] = (*st_1);

					long long int dummy_add = rand()%10000000;
					MemEvent *e = new MemEvent(Owner, dummy_add, dummy_add, GetS);
					SST::Event * ev = e;




					WSR_COUNT[mmu_id] = k-1;
					//		WID_EV[mmu_id] = ev;
					WID_Add[mmu_id] = addr;
					WSR_READY[mmu_id] = false;					

					// Add it to the tracking structure
					MEM_REQ[e->getID()]=mmu_id;

					//					std::cout<<"Sending a new request with address "<<std::hex<<dummy_add<<std::endl;
					// Actually send the event to the cache
					to_mem->send(ev);


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


	std::map<SST::Event *, SST::Cycle_t>::iterator st, en;
	st = ready_by.begin();
	en = ready_by.end();

	while(st!=en)
	{

		bool deleted=false;
		if(st->second <= x)
		{


			uint64_t addr = ((MemEvent*) st->first)->getVirtualAddress();


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

			(*service_back_size)[st->first]=ready_by_size[st->first];

			ready_by.erase(st);

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


		}
		if(deleted)
			st=ready_by.begin();
		else
			st++;

	}



	return false;
}



void PageTableWalker::insert_way(long long int vaddr, int way, int struct_id)
{

	int set=abs_int_Samba((vaddr/page_size[struct_id])%sets[struct_id]);
	tags[struct_id][set][way]=vaddr/page_size[struct_id];


}


// Does the translation and updating the statistics of miss/hit
long long int PageTableWalker::translate(long long int vadd)
{
	return 1;

}



// Find if it exists
bool PageTableWalker::check_hit(long long int vadd, int struct_id)
{


	int set= abs_int_Samba((vadd/page_size[struct_id])%sets[struct_id]);

	for(int i=0; i<assoc[struct_id];i++)
		if(tags[struct_id][set][i]==vadd/page_size[struct_id])
			return true;

	return false;
}

// To insert the translaiton
int PageTableWalker::find_victim_way(long long int vadd, int struct_id)
{

	int set= abs_int_Samba((vadd/page_size[struct_id])%sets[struct_id]);

	for(int i=0; i<assoc[struct_id]; i++)
		if(lru[struct_id][set][i]==(assoc[struct_id]-1))
			return i;


	return 0;
}

// This function updates the LRU policy for a given address
void PageTableWalker::update_lru(long long int vaddr, int struct_id)
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


