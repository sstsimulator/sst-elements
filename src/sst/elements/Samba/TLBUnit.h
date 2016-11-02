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
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>

#include<map>
#include<vector>

// This file defines a TLB structure


class TLB 
{


	int page_size; // By default, lets assume 4KB pages

	int assoc; // This represents the associativiety

	long long int ** tags; // This will hold the tags

	int ** lru; // This will hold the lru positions

	TLB * next_level; // a pointer to the next level Samba structure


	int  sets; //stores the number of sets

	int size; // Number of entries

	int hits; // number of hits

	int misses; // number of misses

	int latency; // indicates the latency in cycles

	int upper_link_latency; // This indicates the upper link latency

	int max_outstanding; // indicates the number of maximum outstanding misses

	int max_width; // indicates the number of maximum accesses on the same cycle

	int parallel_mode; // very specific case for L1 TLB in case of overlapping with accessing the cache

	std::vector<SST::Event *> * service_back; // This is used to pass ready requests back to the previous level

	std::map<SST::Event *, SST::Cycle_t > ready_by; // this one is used to keep track of requests that are delayed inside this structure, compensating for latency

	std::vector<SST::Event *> pushed_back; // This what we got returned from other structures


	std::vector<SST::Event *> pending_misses; // This the number of pending misses, only erased when pushed back from next level

	std::vector<SST::Event *> not_serviced; // This holds those accesses not serviced yet


	int page_walk_latency; // this is really nothing than the page walk latency in case of having no walkers

	public: 

	TLB() {} // For serialization
	TLB(int page_size, int assoc, TLB * next_level, int size);
	TLB(int tlb_id, TLB * Next_level,int level, SST::Component * owner, SST::Params& params);

	// Does the translation and updating the statistics of miss/hit
	long long int translate(long long int vadd);

	void finish(){}

	// Find if it exists
	bool check_hit(long long int vadd);

	// To insert the translaiton
	int find_victim_way(long long int vadd);

	void setServiceBack( std::vector<SST::Event *> * x) { service_back = x;}

	std::vector<SST::Event *> * getPushedBack(){return & pushed_back;}
	void update_lru(long long int vaddr);


	Statistic<uint64_t>* statTLBHits;

	Statistic<uint64_t>* statTLBMisses;


	int getHits(){return hits;}
	int getMisses(){return misses;}

	void insert_way(long long int vaddr, int way);

	// This one is to push a request to this structure
	void push_request(SST::Event * x) { not_serviced.push_back(x);}

	bool tick(SST::Cycle_t x);


};
