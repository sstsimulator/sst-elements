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
#include "TLBUnit.h"


#include<iostream>

using namespace SST::MemHierarchy;
using namespace SST;

TLB::TLB(int Page_size, int Assoc, TLB * Next_level, int Size)
{

	// Initializing the TLB structure

	/*
	   page_size=Page_size;

	   assoc=Assoc;

	   next_level=Next_level;

	   size=Size;

	   hits=misses=0;

	   sets= size/assoc;

	   tags = new long long int*[sets];

	   lru = new int*[sets];

	   for(int i=0; i < sets; i++)
	   {
	   tags[i]=new long long int[assoc];
	   lru[i]=new int[assoc];
	   for(int j=0; j<assoc;j++)
	   {
	   tags[i][j]=-1;
	   lru[i][j]=j;
	   }
	   }
	   */

}


TLB::TLB(int tlb_id, TLB * Next_level, int level, SST::Component * owner, SST::Params& params)
{


	next_level = Next_level;

	std::string LEVEL = std::to_string(level);

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");



	page_walk_latency = ((uint32_t) params.find<uint32_t>("page_walk_latency", 50));

	max_outstanding = ((uint32_t) params.find<uint32_t>("max_outstanding_L"+LEVEL, 4));

	latency = ((uint32_t) params.find<uint32_t>("latency_L"+LEVEL, 1));

	// This indicates the number of page sizes supported for this level
	sizes = ((uint32_t) params.find<uint32_t>("sizes_L"+LEVEL, 1));

	//std::cout<<"The latency of "<<"latency_L"+LEVEL<<"is "<<latency<<std::endl;

	parallel_mode = ((uint32_t) params.find<uint32_t>("parallel_mode_L"+LEVEL, 0));

	upper_link_latency = ((uint32_t) params.find<uint32_t>("upper_link_L"+LEVEL, 1));

	//std::cout<<"Page size is "<<page_size<<std::endl;

	//std::cout<<"Max # of outstanding requests is "<<max_outstanding<<std::endl;

	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "Core%d_L%d", tlb_id,level);


	// The stats that will appear, not that these stats are going to be part of the Samba unit
	statTLBHits = owner->registerStatistic<uint64_t>( "tlb_hits", subID);
	statTLBMisses = owner->registerStatistic<uint64_t>( "tlb_misses", subID );

	max_width = ((uint32_t) params.find<uint32_t>("max_width_L"+LEVEL, 4));


	os_page_size = ((uint32_t) params.find<uint32_t>("os_page_size", 4));

	size = new int[sizes]; 
	assoc = new int[sizes];
	page_size = new int[sizes];
	sets = new int[sizes];
	tags = new long long int**[sizes];
	lru = new int **[sizes];


	for(int i=0; i < sizes; i++)
	{

		size[i] =  ((uint32_t) params.find<uint32_t>("size"+std::to_string(i+1) + "_L"+LEVEL, 1));

		//std::cout<<"Size is "<<size<<std::endl;

		assoc[i] =  ((uint32_t) params.find<uint32_t>("assoc"+std::to_string(i+1) +  "_L"+LEVEL, 1));

		//std::cout<<"Associativity is "<<assoc<<std::endl;

		page_size[i] = 1024 * ((uint32_t) params.find<uint32_t>("page_size"+ std::to_string(i+1) + "_L" + LEVEL, 4));


		// Here we add the supported page size and the structure index
		SIZE_LOOKUP[page_size[i]/1024]=i;


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

	//	registerClock( cpu_clock, new SST::Clock::Handler<TLB>(this, &TLB::tick ) );


}


bool TLB::tick(SST::Cycle_t x)
{


	// pushing back all the requests ready from lower levels
	while(!pushed_back.empty())
	{


		SST::Event * ev = pushed_back.back();
		//service_back->push_back(ev);

		uint64_t addr = ((MemEvent*) ev)->getVirtualAddress();


		// Double checking that we actually still don't have it inserted
		if(SIZE_LOOKUP.find(pushed_back_size[ev])!= SIZE_LOOKUP.end())
		{

			// Find the id of the structure holds those entries of the page size of this request
			int struct_id = SIZE_LOOKUP[pushed_back_size[ev]];
			if(!check_hit(addr, struct_id))
			{
				insert_way(addr, find_victim_way(addr, struct_id), struct_id);
				update_lru(addr, struct_id);
			}

		}
		// Deleting it from pending requests
		std::vector<SST::Event *>::iterator st, en;
		st = pending_misses.begin();
		en = pending_misses.end();


		while(st!=en)
		{
			if(*st == ev)
			{
				pending_misses.erase(st);
				break;
			}
			st++;
		}

		// Note that here we are sustitiuing for latency of checking the tag before proceeing to the next level, we also add the upper link latency for the round trip
		ready_by[ev]= x + latency + 2*upper_link_latency;

		// We also track the size of tthe ready request
		ready_by_size[ev]= pushed_back_size[ev];

		pushed_back_size.erase(ev);
		pushed_back.pop_back();

	}



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
		for(int k=0; k < sizes; k++)
			if(check_hit(addr, k))
			{
				hit=true;
				hit_id=k;
				break;
			}

		if(hit)
		{

			update_lru(addr, hit_id);
			hits++;
			statTLBHits->addData(1);
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

			if(pending_misses.size() < max_outstanding)
			{	
				statTLBMisses->addData(1);
				misses++;
				pending_misses.push_back(*st_1);
				if(next_level!=NULL)
				{

					next_level->push_request(ev);
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

			//	std::cout<<"The request was read at "<<st->second<<" The time now is "<<x<<std::endl;

			uint64_t addr = ((MemEvent*) st->first)->getVirtualAddress();


			if(SIZE_LOOKUP.find(ready_by_size[st->first])!= SIZE_LOOKUP.end())
			{
			// Double checking that we actually still don't have it inserted
			if(!check_hit(addr, SIZE_LOOKUP[ready_by_size[st->first]]))
			{
				insert_way(addr, find_victim_way(addr, SIZE_LOOKUP[ready_by_size[st->first]]), SIZE_LOOKUP[ready_by_size[st->first]]);
				update_lru(addr, SIZE_LOOKUP[ready_by_size[st->first]]);
			}
			else
				update_lru(addr, SIZE_LOOKUP[ready_by_size[st->first]]);
			}



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



void TLB::insert_way(long long int vaddr, int way, int struct_id)
{

	int set=(vaddr/page_size[struct_id])%sets[struct_id];
	tags[struct_id][set][way]=vaddr/page_size[struct_id];


}


// Does the translation and updating the statistics of miss/hit
long long int TLB::translate(long long int vadd)
{
/*
	bool hit= check_hit(vadd);

	if(hit)
	{

		statTLBHits->addData(1);
		hits++;	 
		update_lru(vadd);

	}
	else
	{

		statTLBMisses->addData(1);
		misses++;
		insert_way(vadd, find_victim_way(vadd));
		update_lru(vadd);



	}
*/
	return 1;

}



// Find if it exists
bool TLB::check_hit(long long int vadd, int struct_id)
{


	int set= (vadd/page_size[struct_id])%sets[struct_id];
	for(int i=0; i<assoc[struct_id];i++)
		if(tags[struct_id][set][i]==vadd/page_size[struct_id])

			return true;

	return false;
}

// To insert the translaiton
int TLB::find_victim_way(long long int vadd, int struct_id)
{

	int set= (vadd/page_size[struct_id])%sets[struct_id];

	for(int i=0; i<assoc[struct_id]; i++)
		if(lru[struct_id][set][i]==(assoc[struct_id]-1))
			return i;


	return 0;
}

// This function updates the LRU policy for a given address
void TLB::update_lru(long long int vaddr, int struct_id)
{

	int lru_place=assoc[struct_id]-1;

	int set= (vaddr/page_size[struct_id])%sets[struct_id];
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


