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

#ifndef _H_SST_TLB
#define _H_SST_TLB

#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include "PageTableWalker.h"
#include<map>
#include<vector>

// This file defines a TLB structure

using namespace SST::SambaComponent;

class TLB 
{

	SST::Component *Owner;

	int coreId;

	int level; // This indicates the level of the TLB Unit

	int perfect; // This indidicates if the TLB is perfect or not, perfect is used to measure performance overhead over an ideal TLB hierarchy

	int sizes; // This indicates the number of sizes supported

	int * page_size; // By default, lets assume 4KB pages

	int * assoc; // This represents the associativiety

	Address_t *** tags; // This will hold the tags

	bool ***valid; //This will hold the status of the tags

	int *** lru; // This will hold the lru positions

	TLB * next_level; // a pointer to the next level Samba structure

	PageTableWalker * PTW; // This is a pointer to the PTW in case of being last level

	std::map<long long int, int> SIZE_LOOKUP; // This structure checks if a size is supported inside the structure, and its index structure

	std::map< Address_t, std::map< SST::Event *, int>> SAME_MISS; // This tracks the misses for the same location and deduplicates them
	std::map<Address_t, int> PENDING_MISS; // This tracks the addresses of the current master misses (other contained misses are tracked in SAME_MISS)

	int  * sets; //stores the number of sets

	int* size; // Number of entries

	int hits; // number of hits

	int misses; // number of misses

	int emulate_faults; // If set, then page faults will send requests to Opal

	int os_page_size; // This is a hack for the size of the frames returned by the OS, by default

	int latency; // indicates the latency in cycles

	int upper_link_latency; // This indicates the upper link latency

	int max_outstanding; // indicates the number of maximum outstanding misses

	int max_width; // indicates the number of maximum accesses on the same cycle

	int parallel_mode; // very specific case for L1 TLB in case of overlapping with accessing the cache

	std::vector<SST::Event *> * service_back; // This is used to pass ready requests back to the previous level

	std::map<SST::Event *, long long int> * service_back_size; // This is used to pass the size of the  requests back to the previous level

	std::map<SST::Event *, SST::Cycle_t > ready_by; // this one is used to keep track of requests that are delayed inside this structure, compensating for latency

	std::map<SST::Event *, long long int> ready_by_size; // this one is used to keep track of requests' sizes inside this structure

	std::vector<SST::Event *> pushed_back; // This is what we got returned from other structures

	std::map<SST::Event *, long long int> pushed_back_size; // This is the sizes of the translations we got returned from other structures

	std::vector<SST::Event *> pending_misses; // This the number of pending misses, only erased when pushed back from next level

	std::vector<SST::Event *> not_serviced; // This holds those accesses not serviced yet


	int page_walk_latency; // this is really nothing than the page walk latency in case of having no walkers

	public: 

	TLB() {} // For serialization
	TLB(int page_size, int assoc, TLB * next_level, int size);
	TLB(int tlb_id, TLB * Next_level,int level, SST::Component * owner, SST::Params& params);
	TLB(int tlb_id, PageTableWalker * Next_level, int level, SST::Component * owner, SST::Params& params);
	// Does the translation and updating the statistics of miss/hit
	Address_t translate(Address_t vadd);

	void finish(){}

	// Invalidate TLB entry
	void invalidate(Address_t vadd);

	// Find if it exists
	bool check_hit(Address_t vadd, int struct_id);

	// To insert the translaiton
	int find_victim_way(Address_t vadd, int struct_id);

	void setServiceBack( std::vector<SST::Event *> * x) { service_back = x;}

	void setServiceBackSize( std::map<SST::Event *, long long int> * x) { service_back_size = x;}

	std::vector<SST::Event *> * getPushedBack(){return & pushed_back;}

	std::map<SST::Event *, long long int> * getPushedBackSize(){return & pushed_back_size;}

	void update_lru(Address_t vaddr, int struct_id);


	Statistic<uint64_t>* statTLBHits;

	Statistic<uint64_t>* statTLBMisses;

	Statistic<uint64_t>* statTLBShootdowns;


	int getHits(){return hits;}
	int getMisses(){return misses;}

	void insert_way(Address_t vaddr, int way, int struct_id);

	// This one is to push a request to this structure
	void push_request(SST::Event * x) { not_serviced.push_back(x);}

	bool tick(SST::Cycle_t x);


};

#endif
