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


/*
#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
*/

#include<map>
#include <cstddef>
#include<vector>
#include "WriteBuffer.h"


using namespace SST;
using namespace SST::MessierComponent;

// In this file, we define the main functions for the writebuffer structure

// returns true if the number of entries exceeds the threshold
bool NVM_WRITE_BUFFER::flush()
{

	return still_flushing;

}

// Insert a write request entry in the write buffer
bool NVM_WRITE_BUFFER::insert_write_request(NVM_Request * req)
{



	if(curr_entries < max_size)
	{

		ADD_REQ[req->Address/entry_size]=req;
		mem_reqs.push_back(req);
		curr_entries++;


		if(mem_reqs.size() != curr_entries)
			std::cout<<"Massive error 1"<<std::endl;


		if( curr_entries >= (flush_th*1.0*max_size/100.0) )
			still_flushing=true;


		return true;
	}
	else
		return false;


}


// Finding an entry request, mainly to check if the request exists on write-buffer before proceeding to submit the request to the memory
NVM_Request * NVM_WRITE_BUFFER::find_entry(long long int address)
{

	// Fast path: note that this is the common case where there is no entry in WB, hence speeding up SST time
	if(ADD_REQ.find(address/entry_size) == ADD_REQ.end())
		return NULL;
	else
		return ADD_REQ[address/entry_size];

}

// Popping up the first entry in the write buffer, this is called by the NVM memory controller when it is idle or the flush signal is triggered in the write buffer
NVM_Request * NVM_WRITE_BUFFER::pop_entry()
{
	if(mem_reqs.empty())
		return NULL;

	NVM_Request * TEMP = mem_reqs.front();
	ADD_REQ.erase(TEMP->Address/entry_size);
	mem_reqs.pop_front();
	curr_entries--;
	
		if(mem_reqs.size() != curr_entries)
			std::cout<<"Massive error 2"<<std::endl;

	if(curr_entries <= (flush_th_low*1.0*max_size/100.0) )
		still_flushing=false;

	return TEMP;
}


// Towards out-of-order execution of writes. With this we enable the memory controller to execute and erase any write request
void NVM_WRITE_BUFFER::erase_entry(NVM_Request * TEMP)
{

	ADD_REQ.erase(TEMP->Address/entry_size);
	mem_reqs.remove(TEMP);
	curr_entries--;

		if(mem_reqs.size() != curr_entries)
			std::cout<<"Massive error 3  curr_entrie="<<curr_entries<<"  mem_reqs.size()= "<<mem_reqs.size()<<std::endl;

	 if(curr_entries <= (flush_th_low*1.0*max_size/100.0) )
                still_flushing=false;

}


