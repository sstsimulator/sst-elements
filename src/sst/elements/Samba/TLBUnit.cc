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
#include "TLBUnit.h"


#include<iostream>

using namespace SST::MemHierarchy;
using namespace SST;

// Find the absoulute value
int abs_int(int x)
{

	if(x<0)
		return -1*x;
	else
		return x;

}


// Not needed for now, probably will be removed soon, if doesn't cause any compaitability issues
TLB::TLB(int Page_size, int Assoc, TLB * Next_level, int Size)
{

}



// The constructer mainly used when creating TLBUnits
TLB::TLB(int tlb_id, TLB * Next_level, int Level, SST::Component * owner, SST::Params& params)
{

	Owner = owner;

	coreId = tlb_id;

	level = Level;

	next_level = Next_level;

	std::string LEVEL = std::to_string(level);

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");



	page_walk_latency = ((uint32_t) params.find<uint32_t>("page_walk_latency", 50));

	max_outstanding = ((uint32_t) params.find<uint32_t>("max_outstanding_L"+LEVEL, 4));

	emulate_faults = ((uint32_t) params.find<uint32_t>("emulate_faults", 0));

	latency = ((uint32_t) params.find<uint32_t>("latency_L"+LEVEL, 1));

	// This indicates the number of page sizes supported for this level
	sizes = ((uint32_t) params.find<uint32_t>("sizes_L"+LEVEL, 1));

	parallel_mode = ((uint32_t) params.find<uint32_t>("parallel_mode_L"+LEVEL, 0));

	upper_link_latency = ((uint32_t) params.find<uint32_t>("upper_link_L"+LEVEL, 0));


	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "Core%d_L%d", tlb_id,level);


	// The stats that will appear, not that these stats are going to be part of the Samba unit
	statTLBHits = owner->registerStatistic<uint64_t>( "tlb_hits", subID);
	statTLBMisses = owner->registerStatistic<uint64_t>( "tlb_misses", subID );
	statTLBShootdowns = owner->registerStatistic<uint64_t>( "tlb_shootdown", subID );

	max_width = ((uint32_t) params.find<uint32_t>("max_width_L"+LEVEL, 4));


	perfect = ((uint32_t) params.find<uint32_t>("perfect", 0));

	os_page_size = ((uint32_t) params.find<uint32_t>("os_page_size", 4));

	size = new int[sizes]; 
	assoc = new int[sizes];
	page_size = new int[sizes];
	sets = new int[sizes];
	tags = new Address_t**[sizes];
	valid = new bool**[sizes];
	lru = new int **[sizes];


	for(int i=0; i < sizes; i++)
	{

		size[i] =  ((uint32_t) params.find<uint32_t>("size"+std::to_string(i+1) + "_L"+LEVEL, 1));


		assoc[i] =  ((uint32_t) params.find<uint32_t>("assoc"+std::to_string(i+1) +  "_L"+LEVEL, 1));


		page_size[i] = 1024 * ((uint32_t) params.find<uint32_t>("page_size"+ std::to_string(i+1) + "_L" + LEVEL, 4));


		// Here we add the supported page size and the structure index
		SIZE_LOOKUP[page_size[i]/1024]=i;


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
				valid[id][i][j]=true;
				lru[id][i][j]=j;
			}
		}

	}

	//	registerClock( cpu_clock, new SST::Clock::Handler<TLB>(this, &TLB::tick ) );


}

// This constructer is only used for creating the last-level TLB Unit, hence the PageTableWalker argument passed to it
TLB::TLB(int tlb_id, PageTableWalker * Next_level, int level, SST::Component * owner, SST::Params& params)
{

	std::cout<<"Assigning the PTW correctly"<<std::endl;

	PTW=Next_level;

	new (this) TLB(tlb_id, (TLB *) NULL, level, owner, params);

}


// This is the most important function, which works like the heart of the TLBUnit, called on every cycle to check if any completed requests or new requests at this cycle.
bool TLB::tick(SST::Cycle_t x)
{


	// pushing back all the requests ready from lower levels
	while(!pushed_back.empty())
	{


		SST::Event * ev = pushed_back.back();

		Address_t addr = ((MemEvent*) ev)->getVirtualAddress();



		// Double checking that we actually still don't have it inserted
		// Insert the translation into all structures with same or smaller size page support. Note that smaller page sizes will still have the same translation with offset derived from address
		std::map<long long int, int>::iterator lu_st, lu_en;
		lu_st=SIZE_LOOKUP.begin();
		lu_en=SIZE_LOOKUP.end();
		while(lu_st!=lu_en)
		{
			if(pushed_back_size[ev] >= lu_st->first)
			{
				if(!check_hit(addr, lu_st->second))
				{
					insert_way(addr, find_victim_way(addr, lu_st->second), lu_st->second);
					update_lru(addr, lu_st->second);
				}

			}

			lu_st++;
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


		// Check if there are other misses that were going to the same translation and waiting for the response of this miss
		if((level==1) && (SAME_MISS.find(addr/4096)!=SAME_MISS.end()))
		{
		  std::map< SST::Event *, int>::iterator same_st, same_en; 
		  same_st = SAME_MISS[addr/4096].begin();
    		  same_en = SAME_MISS[addr/4096].end();
   		   while(same_st!=same_en)
		    {

	    		ready_by[same_st->first] = x + latency + 2*upper_link_latency;
	    		ready_by_size[same_st->first] = pushed_back_size[ev];
			same_st++;
		    }	
		  SAME_MISS.erase(addr/4096);
		 // PENDING_MISS.erase(addr/4096);
		}
		PENDING_MISS.erase(addr/4096);

		pushed_back_size.erase(ev);
		pushed_back.pop_back();

	}



	// The actual dipatching process... here we take a request and place it in the right queue based on being miss or hit and the number of pending misses
	std::vector<SST::Event *>::iterator st_1,en_1;
	st_1 = not_serviced.begin();
	en_1 = not_serviced.end();

	int dispatched=0;

	// Iteravte over the requests passed from higher levels
	for(;st_1!=not_serviced.end(); st_1++)
	{
		dispatched++;

		// Checking that we never dispatch more accesses than the port width of accesses per cycle
		if(dispatched > max_width)
			break;

		SST::Event * ev = *st_1; 
		Address_t addr = ((MemEvent*) ev)->getVirtualAddress();


		// Those track if any hit in one of the supported pages' structures
		bool hit=false;
		int hit_id=0;

		// We check the structures in parallel to find if it hits
		for(int k=0; k < sizes; k++)
			if(check_hit(addr, k) || (perfect==1))
			{
				hit=true;
				hit_id=k;
				break;
			}

		// If it hist in any page size structure, we update the lru position of the translation and update statistics
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
		else // We know the translation doesn't exist, so we need to service a miss
		{

			// Making sure we have a room for an additional miss, i.e., less than the maximum outstanding misses
			if((int) pending_misses.size() < (int) max_outstanding)
			{	

				// Check if the miss is not currently being handled
				bool currently_handled=false;
				if((level==1) && (PENDING_MISS.find(addr/4096) != PENDING_MISS.end()))
				{

					SAME_MISS[addr/4096][ev]=1; // Just adding it to the 2D map, so we later hand it back once the master miss is complete
					currently_handled = true;
				}
				else if(level==1)
				{

					PENDING_MISS[addr/4096]=1;

				}

				statTLBMisses->addData(1);
				misses++;

				if(!currently_handled)
				{

					pending_misses.push_back(*st_1);
					// Check if the last level TLB or not, if last-level, pass the request to the page table walker
					if(next_level!=NULL)
					{

						next_level->push_request(ev);
						st_1 = not_serviced.erase(st_1);
					}
					else // Passs it to the page table walker
					{
						PTW->push_request(ev);
						st_1 = not_serviced.erase(st_1);
					}
				}
				else
				{
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

	// We iterate over the list of being serviced request to see if any has finished by this cycle
	while(st!=en)
	{

		bool deleted=false;
		if(st->second <= x)
		{

			//	std::cout<<"The request was read at "<<st->second<<" The time now is "<<x<<std::endl;

			Address_t addr = ((MemEvent*) st->first)->getVirtualAddress();


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


// Used to insert a new translation on a specific way of the TLB structure
void TLB::insert_way(Address_t vaddr, int way, int struct_id)
{

	int set=abs_int((vaddr/page_size[struct_id])%sets[struct_id]);
	tags[struct_id][set][way]=vaddr/page_size[struct_id];
	valid[struct_id][set][way]=true;

}



// This function will be used later to get the translation of a specific virtual address (if cached)
Address_t TLB::translate(Address_t vadd)
{
	return 1;

}



// Invalidate TLB entries
void TLB::invalidate(Address_t vadd)
{

	for(int id=0; id<sizes; id++)
	{
		//std::cout << Owner->getName().c_str() << " TLB " << coreId << " id: " << id << " invalidate address: " << vadd << " index: " << vadd*page_size[0]/page_size[id] << std::endl;
		int set= abs_int((vadd*page_size[0]/page_size[id])%sets[id]);
		for(int i=0; i<assoc[id]; i++) {
			if(tags[id][set][i]==vadd*page_size[0]/page_size[id] && valid[id][set][i]) {
				//std::cout << Owner->getName().c_str() << " TLB " << coreId << " invalidate address: " << vadd << " index: " << vadd*page_size[0]/page_size[id] << " found" << std::endl;
				valid[id][set][i] = false;
				break;
			}
		}
	}

	statTLBShootdowns->addData(1);
}



// Find if the translation  exists on structure struct_id
bool TLB::check_hit(Address_t vadd, int struct_id)
{


	int set= abs_int((vadd/page_size[struct_id])%sets[struct_id]);
	for(int i=0; i<assoc[struct_id];i++)
		if(tags[struct_id][set][i]==vadd/page_size[struct_id])
			return valid[struct_id][set][i];

	return false;
}

// To insert the translaiton
int TLB::find_victim_way(Address_t vadd, int struct_id)
{

	int set= abs_int((vadd/page_size[struct_id])%sets[struct_id]);

	for(int i=0; i<assoc[struct_id]; i++)
		if(lru[struct_id][set][i]==(assoc[struct_id]-1))
			return i;


	return 0;
}

// This function updates the LRU position for a given address of a specific structure
void TLB::update_lru(Address_t vaddr, int struct_id)
{

	int lru_place=assoc[struct_id]-1;

	int set= abs_int((vaddr/page_size[struct_id])%sets[struct_id]);
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


