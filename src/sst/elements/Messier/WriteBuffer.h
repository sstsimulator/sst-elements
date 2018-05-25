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


//
/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */

#ifndef _H_SST_NVM_WB
#define _H_SST_NVM_WB


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include<map>
#include<list>
#include "NVM_Request.h"

using namespace SST;

// This class structure represents the write buffer unit inside NVM memory controller 

namespace SST{ namespace MessierComponent {
class NVM_WRITE_BUFFER
{ 

	// This determines the size in number of entries
	unsigned int max_size;

	// scheduling mode: this can be used to optimize writing to NVMs, cache lines within the same rowbuffer are prioritized to be written together
	int sched_mode;

	// flush threshold percentage; this determines at what percentage of max number of entries triggers flushing the write buffer: e.g., 50% means when we have 50% of the size entries, we should attempt flushing
	int flush_th;

	// This indicates the low threshold, where the controller will bring down the number of entries to, whenever it reaches the high threshold
	int flush_th_low;

	// the current number of entries
	unsigned int curr_entries;

	// This tracks them in order
	std::list<NVM_Request *> mem_reqs;


	// This is used to speed up returning the memory requests in case of finding the request in the write buffer
	std::map<long long int, NVM_Request *> ADD_REQ;

	int entry_size; // this determines the granularity of the write requests, ideally this should be similar to cache line size

	bool still_flushing; // This indicates that the controller is still trying to bring down the entries to low threshold

	public:

	

	// Constructor
	NVM_WRITE_BUFFER(int Size, int Sched_mode, int Entry_size, int Flush_th, int low_th){ flush_th_low = low_th; max_size = Size; sched_mode = Sched_mode; flush_th = Flush_th; entry_size = Entry_size; still_flushing=false; curr_entries = 0;}

	// This checks if the writebuffer is in the flush mode (entries exceed threshold)
	bool flush();

	// Get the list size
	int ListSize() { return mem_reqs.size();}

	// Check if empty
	bool empty() { if (curr_entries == 0) return true; else return false;}
	// Get the current size
	int getSize(){ return curr_entries;}

	// Tells you if larger than the low threshold;
	
	// Check if full or not
	bool full() { if(curr_entries == max_size) return true; else return false;}

	// Insert an entry (returns false if it fails, otherwise it return true)
	bool insert_write_request(NVM_Request * req);

        // This enables searching if a request exists on the write buffer (this is important for correctness and not to break memory consistency)
        NVM_Request * find_entry(long long int address);		

	// This removes an entry (returns NULL if empty)
	NVM_Request * pop_entry();

	NVM_Request * getFront() { return mem_reqs.front();}

	void erase_entry(NVM_Request *);	

	std::list<NVM_Request *> getList() { return mem_reqs;}


};
}}
#endif
