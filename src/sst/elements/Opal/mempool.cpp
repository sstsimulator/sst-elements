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
 * E-mail: amro.awad@ucf.edu
 */
/* Author: Vamsee Reddy Kommareddy
 * E-mail: vamseereddy@knights.ucf.edu
 */

#include "mempool.h"


//Constructor for pool
Pool::Pool(SST::Component* own, Params params, SST::OpalComponent::MemType mem_type, int id)
{

	owner = own;

	output = new SST::Output("OpalMemPool[@f:@l:@p] ", 16, 0, SST::Output::STDOUT);

	size = params.find<long long int>("size", 0); // in KB's

	start = params.find<long long int>("start", 0);

	frsize = params.find<int>("frame_size", 4); //4KB frame size

	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "%" PRIu32, id);

	memType = mem_type;
	if(memType == SST::OpalComponent::MemType::LOCAL) {
		localMemID = id;
		memUsage = own->registerStatistic<uint64_t>( "local_mem_usage", subID );
	}
	else {
		sharedMemID = id;
		memUsage = own->registerStatistic<uint64_t>( "shared_mem_usage", subID );
	}

	free(subID);

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
	Frame *frame;
	num_frames = ceil(size/frsize);
	real_size = num_frames * frsize;

	for(i=0; i< num_frames; i++) {
		frame = new Frame(((long long int) i*frsize*1024) + start, 0);
		freelist.push_back(frame);
	}

	available_frames = num_frames;

	return;

}

MemPoolResponse Pool::allocate_frames(int _size)
{

	MemPoolResponse mpr;
	int frames = ceil(_size/(frsize*1024));
	std::list<Frame*> frames_allocated;

	if(available_frames < frames) {
		mpr.pAddress = -1;
		return mpr;
	}

	// Fixme: Shuffle memory to make continuous memory available
	while(frames) {
		// Make sure pool has free frames in the requested memory pool type. If not deallocate allocated frames (if number of frames to be allocated are > 1)
		if(freelist.empty()) {
			while(!frames_allocated.empty()) {
				Frame *frame = frames_allocated.front();
				freelist.push_back(frame);
				alloclist.erase(frame->starting_address);
				frames_allocated.pop_front();
				available_frames++;
			}
			mpr.pAddress = -1;
			break;
		}
		else
		{
			Frame *frame = freelist.front();
			freelist.pop_front();
			alloclist[frame->starting_address] = frame;
			frames_allocated.push_back(frame);
			available_frames--;
		}
		frames--;

	}

	if(!frames_allocated.empty()) {
		mpr.pAddress = ((Frame *)frames_allocated.front())->starting_address;
		mpr.num_frames = ceil(_size/(frsize*1024));
		mpr.frame_size = frsize;
		memUsage->addData(mpr.num_frames);

	}
	else
		mpr.pAddress = -1;

	return mpr;

}

// Allocate N contigiuous frames, returns the starting address if successfull, or -1 if it fails!
MemPoolResponse Pool::allocate_frame(int N)
{

	MemPoolResponse mpr;
	// Make sure we have free frames first
	if(freelist.empty()) {
		mpr.pAddress = -1;
		return mpr;
	}
	// For now, we will assume you can only allocate 1 frame, TODO: We will implemenet a buddy-allocator style that enables allocating contigous physical spaces
	if(N>1) {
		mpr.pAddress = -1;
		return mpr;
	}
	else
	{
		// Simply, pop the first free frame and assign it
		Frame * temp = freelist.front();
		freelist.pop_front();
		alloclist[temp->starting_address] = temp;
		memUsage->addData(1);
		mpr.pAddress = temp->starting_address;
		mpr.frame_size = frsize;
		mpr.num_frames = 1;
		return mpr;

	}

}

/* Deallocate 'size' contigiuous memory of type 'memType' starting from physical address 'starting_pAddress',
 * returns a structure which indicates whether the memory is successfully deallocated or not
 */
MemPoolResponse Pool::deallocate_frames(int _size, long long int starting_pAddress)
{

	MemPoolResponse mpr;
	int frames = ceil(_size/frsize);
	long long int pAddress = starting_pAddress;
	uint64_t frame_number;

	mpr.frame_size = frsize;

	while(frames) {

		// If we can find the frame to be free in the allocated list
		std::map<long long int, Frame*>::iterator it;
		it = alloclist.find(pAddress);
		if (it != alloclist.end())
		{
			//Remove from allocation map and add to free list
			Frame *temp = it->second;
			freelist.push_back(temp);
			alloclist.erase(pAddress);

		}
		else
		{
			mpr.pAddress = pAddress; //physical address of the frame which failed to deallocate.
			mpr.num_frames = frames; //This indicates number of frames that are not deallocated.
			return mpr;
		}

		frame_number = (pAddress - start) / frsize * 1024;
		pAddress += ((long long int) (frame_number+1)*frsize*1024) + start; //to get the next frame physical address
		frames--;
	}

	mpr.pAddress = -1; //successfully deallocated
	mpr.num_frames = frames;
	return mpr;
}

// Freeing N frames starting from Address X, this will return -1 if we find that these frames were not allocated
int Pool::deallocate_frame(long long int X, int N)
{


	// For now, we will assume you can free only 1 frame, TODO: We will implemenet a buddy-allocator style that enables allocating and freeing contigous physical spaces
	if(N>=1)
		return -1;
	else
	{
		// If we can find the frame to be free in the allocated list
		if(alloclist.find(X)!=alloclist.end())
		{
			// Remove from allocation map and add to free list
			Frame * temp = alloclist[X];
			freelist.push_back(temp);
			alloclist.erase(X);

		}
		else // Means we couldn't find an allocated frame that is being unmapped
			return -1;

	}

	return 0;
}



