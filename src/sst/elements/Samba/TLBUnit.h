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

#ifndef _H_SST_TLB
#define _H_SST_TLB

#include <sst_config.h>
#include <sst/core/componentExtension.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include "PageTableWalker.h"
#include <map>
#include <vector>
#include "utils.h"

// This file defines a TLB structure

using namespace SST::SambaComponent;

class TLB : public ComponentExtension
{

	int coreId;

	int level; // This indicates the level of the TLB Unit

    // === Params
	int perfect; // This indidicates if the TLB is perfect or not, perfect is used to measure performance overhead over an ideal TLB hierarchy
	int sizes; // number of pages sizes (i.e. 3 for 4kb+2MB+1GB) supported in this level of TLB

	int emulate_faults;     // If set, then page faults will send requests to page fault handler
	int os_page_size;       // This is a hack for the size of the frames returned by the OS, by default
	int latency;            // latency in cycles
	int upper_link_latency; // upper link latency (in cycles?)
	int max_outstanding;    // number of maximum outstanding misses
	int max_width;          // number of maximum accesses on the same cycle
	int parallel_mode;      // very specific case for L1 TLB in case of overlapping with accessing the cache
	int page_walk_latency;  // this is really nothing other than the page walk latency in case of having no walkers

    // === Per page-type params (set separately for each separate type of huge-page of huge-pages)
	uint64_t * page_size; // for each [pg-type], size of that page in bytes (so the standard 4k/2M/1G would be 1024 * [4,2048,1048576]

	int * size;  // Number of TLB entries, indexed by [pg-type]
	int * assoc; // associativity of entries, for     [pg-type]
	int * sets;  // stores the number of sets, by     [pg-type]

    // === Cache data for TLB entries
    // - separate cache for each size of page
    // - accessed as `tags[page_size][set][way]`
	Address_t *** tags;
	bool ***valid; // status of the tags
	int *** lru;   // lru positions


    // === Counters
	int hits; // number of hits
	int misses; // number of misses


    // === Links to other levels
	TLB * next_level; // a pointer to the next level Samba structure
	PageTableWalker * PTW; // This is a pointer to the PTW in case of being last level


    // === ???
	std::map<long long int, int> SIZE_LOOKUP; // This structure checks if a size is supported inside the structure, and its index structure

	std::map< Address_t, std::map< MemHierarchy::MemEventBase *, int, MemEventPtrCompare>> SAME_MISS; // This tracks the misses for the same location and deduplicates them
	std::map<Address_t, int> PENDING_MISS; // This tracks the addresses of the current master misses (other contained misses are tracked in SAME_MISS)


    //=======================================================================
    //
    //           BUFFERS FOR REQUESTS TRAVELLING UP AND DOWN TLBS
    //
    //=======================================================================

    //  note: the _size buffers holds the size of the relevant page,
    //  i.e 1 for 4k, 2 for 2M, 3 for 1GB (if using standard page sizes)

    // === Holds incoming requests, "input queue"
	std::vector<MemHierarchy::MemEventBase *> not_serviced;

    // === Holds a copy of requests that have missed in this level, and have been sent into the next level down.
    //  events are erased when they are fulfilled, and returned into `this->pushed_back`
	std::vector<MemHierarchy::MemEventBase *> pending_misses; 

    // === Holds requests that have gotten the data they need, but we need to wait the duration of the latency before returning
	std::map<MemHierarchy::MemEventBase *, SST::Cycle_t, MemEventPtrCompare> ready_by; 
	std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> ready_by_size; // keeps track of requests' sizes inside this structure


    // === Buffers for sending requests up/down TLB hierarchy:
    
    // requests sent lower in TLB hierarchy, will be returned into `this->pushed_back` by lower levels
	std::vector<MemHierarchy::MemEventBase *> pushed_back; // translation for requests, returned from lower-level structures
	std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> pushed_back_size; // page_sizes of the returned translations

    // when we're finished with a request, we send it back up the hierarchy by inserting into `service_back`
    // - pointer is wired up to `pushed_back` buffers of the next level up at TLB in constructor of TLBHierarchy
	std::vector<MemHierarchy::MemEventBase *> * service_back; // used to pass ready requests back to the previous level
	std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> * service_back_size; // page_size of ready requests for next level up



	public:

	TLB(ComponentId_t id, int page_size, int assoc, TLB * next_level, int size);
	TLB(ComponentId_t id, int tlb_id, TLB * Next_level,int level, SST::Params& params);

        // Set PTW for last level
        void setPTW(PageTableWalker * ptw);

	// Does the translation and updating the statistics of miss/hit
	Address_t translate(Address_t vadd);

	void finish(){}

	// Invalidate TLB entry
	void invalidate(Address_t vadd, int id);

	// Find if it exists
	bool check_hit(Address_t vadd, int struct_id);

	// To insert the translaiton
	int find_victim_way(Address_t vadd, int struct_id);

    // === Called by parent to wire up TLB levels to each other
    // this TLB will push completed requests into service_back (sending them back up the levels towards core)
	void setServiceBack( std::vector<MemHierarchy::MemEventBase *> * x) { service_back = x;}
	void setServiceBackSize( std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> * x) { service_back_size = x;}

    // lower-levels will return answered requests into this->pushed_back
	std::vector<MemHierarchy::MemEventBase *> * getPushedBack(){return & pushed_back;}
	std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> * getPushedBackSize(){return & pushed_back_size;}

	void update_lru(Address_t vaddr, int struct_id);


	Statistic<uint64_t>* statTLBHits;

	Statistic<uint64_t>* statTLBMisses;

	Statistic<uint64_t>* statTLBShootdowns;


	int getHits(){return hits;}
	int getMisses(){return misses;}

	void insert_way(Address_t vaddr, int way, int struct_id);

	// This one is to push a request to this structure
	void push_request(MemHierarchy::MemEventBase * x) { not_serviced.push_back(x);}

	bool tick(SST::Cycle_t x);


};

#endif
