// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#include <sst_config.h>

#include "mempool.h"

#include <algorithm>
#include <chrono>

//Constructor for pool
Pool::Pool(Params params, SST::OpalComponent::MemType mem_type, int id)
{

	output = new SST::Output("OpalMemPool[@f:@l:@p] ", 16, 0, SST::Output::STDOUT);

	size = params.find<uint32_t>("size", 0); // in KB's

	start = params.find<uint64_t>("start", 0);

	frsize = params.find<int>("frame_size", 4); //4KB frame size

	memType = mem_type;

	poolId = id;

	/*char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "%" PRIu32, id);

	if(memType == SST::OpalComponent::MemType::LOCAL) {
		memUsage = own->registerStatistic<uint64_t>( "local_mem_usage", subID );
		mappedMemory = own->registerStatistic<uint64_t>( "local_mem_mapped", subID );
		unmappedMemory = own->registerStatistic<uint64_t>( "local_mem_unmapped", subID );
		tlbShootdowns = own->registerStatistic<uint64_t>( "tlb_shootdowns", subID );
		tlbShootdownDelay = own->registerStatistic<uint64_t>( "tlb_shootdown_delay", subID );
	}
	else {
		memUsage = own->registerStatistic<uint64_t>( "shared_mem_usage", subID );
		mappedMemory = own->registerStatistic<uint64_t>( "shared_mem_mapped", subID );
		unmappedMemory = own->registerStatistic<uint64_t>( "shared_mem_unmapped", subID );
	}

	free(subID);
	*/
	/* memory technology
	 * 0: DRAM
	 * 1: NVRAM
	 */
	uint32_t mem_tech = (uint32_t) params.find<uint64_t>("mem_tech", 0);
	switch(mem_tech)
	{
	case 1:
		memTech = SST::OpalComponent::MemTech::NVM;
		break;
	case 0:
	default:
		memTech = SST::OpalComponent::MemTech::DRAM;
	}

	std::cerr << "Pool start: " << start << " size: " << size << " frame size: " << frsize << " mem tech: " << mem_tech << std::endl;
	build_mem();

}

//Create free frames of size framesize, note that the size is in KB
void Pool::build_mem()
{
	int i=0;
	num_frames = ceil(size/frsize);
	real_size = num_frames * frsize;

	//std::vector<int> numbers;
	//for(int i=0; i<num_frames; i++)       // add 0-99 to the vector
    //	numbers.push_back(i);

    // random allocation of frames
	//unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	//std::shuffle(numbers.begin(), numbers.end(), std::default_random_engine(seed));

	for(i=0; i< num_frames; i++) {
		freelist.push_back(((uint64_t) i*frsize*1024) + start);
	}

	available_frames = num_frames;

	return;

}

REQRESPONSE Pool::allocate_frames(int pages)
{

	REQRESPONSE response;
	response.status =0;

	if(available_frames < pages) {
		return response;
	}

	int frames = pages;
	std::list<Frame*> frames_allocated;

	// Fixme: Shuffle memory to make continuous memory available
	while(frames) {
		// Make sure pool has free frames in the requested memory pool type. If not deallocate allocated frames (if number of frames to be allocated are > 1)
		if(freelist.empty()) {
			while(!frames_allocated.empty()) {
				Frame *frame = frames_allocated.front();
				freelist.push_back(frame->starting_address);
				alloclist.erase(frame->starting_address);
				frames_allocated.pop_front();
				available_frames++;
				delete frame;
			}
			break;
		}
		else
		{
			uint64_t frameAddr = freelist.front();
			freelist.erase(freelist.begin());
			Frame *frame = new Frame(frameAddr,0);
			alloclist[frame->starting_address] = frame;
			frames_allocated.push_back(frame);
			available_frames--;
		}
		frames--;

	}

	if(!frames_allocated.empty()) {
		response.address = (frames_allocated.front())->starting_address;
		response.pages = pages;
		response.status = 1;
	}

	return response;

}

// Allocate N contigiuous frames, returns the starting address if successfull, or -1 if it fails!
REQRESPONSE Pool::allocate_frame(int N)
{

	REQRESPONSE response;
	response.status = 0;


	// Make sure we have free frames first
	if(freelist.empty())
		return response;

	// For now, we will assume you can only allocate 1 frame, TODO: We will implemenet a buddy-allocator style that enables allocating contigous physical spaces
	if(N>1)
		return response;

	else
	{
		// Simply, pop the first free frame and assign it
		uint64_t frameAddr = freelist.front();
		freelist.erase(freelist.begin());
		Frame *temp = new Frame(frameAddr,0);
		alloclist[temp->starting_address] = temp;
		available_frames--;
		response.address = temp->starting_address;
		response.pages = 1;
		response.status = 1;
		return response;

	}

}

/* Deallocate 'size' contigiuous memory of type 'memType' starting from physical address 'starting_pAddress',
 * returns a structure which indicates whether the memory is successfully deallocated or not
 */
REQRESPONSE Pool::deallocate_frames(int pages, uint64_t starting_pAddress)
{

	REQRESPONSE response;
	int frames = pages;
	uint64_t pAddress = starting_pAddress;
	uint64_t frame_number;

	while(frames) {

		// If we can find the frame to be free in the allocated list
		std::map<uint64_t, Frame*>::iterator it;
		it = alloclist.find(pAddress);
		if (it != alloclist.end())
		{
			//Remove from allocation map and add to free list
			Frame *temp = it->second;
			freelist.push_back(temp->starting_address);
			alloclist.erase(pAddress);
			delete temp;
		}
		else
		{
			response.address = pAddress; //physical address of the frame which failed to deallocate.
			response.pages = frames; //This indicates number of frames that are not deallocated.
			response.status = 0;
			return response;
		}

		frame_number = (pAddress - start) / frsize * 1024;
		pAddress += ((uint64_t) (frame_number+1)*frsize*1024) + start; //to get the next frame physical address
		frames--;
	}

	response.status = 1; //successfully deallocated
	return response;
}

// Freeing N frames starting from Address X, this will return -1 if we find that these frames were not allocated
REQRESPONSE Pool::deallocate_frame(uint64_t X, int N)
{

	REQRESPONSE response;
	response.status = 0;


	// For now, we will assume you can free only 1 frame, TODO: We will implemenet a buddy-allocator style that enables allocating and freeing contigous physical spaces
	if(N>1)
		return response;
	else
	{
		// If we can find the frame to be free in the allocated list
		if(alloclist.find(X)!=alloclist.end())
		{
			// Remove from allocation map and add to free list
			Frame * temp = alloclist[X];
			freelist.push_back(temp->starting_address);
			alloclist.erase(X);
			delete temp;
			available_frames++;
			response.status = 1;
		}
		else // Means we couldn't find an allocated frame that is being unmapped
			response.status = 0;

	}

	return response;
}

bool Pool::isAllocated(uint64_t address)
{
	if(alloclist.find(address)==alloclist.end())
		return false;

	return true;
}

/*REQRESPONSE Pool::allocate_frame_address(uint64_t address)
{

	REQRESPONSE response;
	response.status = 0;

	// Make sure we have free frames first
	if(freelist.empty())
		return response;

	// For now, we will assume you can only allocate 1 frame, TODO: We will implemenet a buddy-allocator style that enables allocating contigous physical spaces
	if(N>1)
		return response;

	else
	{
		int index = freelist_index[address];
		Frame * temp = freelist[index];
		//freelist.remove(temp);
		freelist.erase(freelist.begin()+index);
		alloclist[temp->starting_address] = temp;
		available_frames--;
		response.address = temp->starting_address;
		response.pages = 1;
		response.status = 1;
		return response;

	}

}*/


