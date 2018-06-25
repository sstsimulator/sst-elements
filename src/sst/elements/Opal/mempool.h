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

#include<list>
#include<map>
#include<cmath>

#include "Opal_Event.h"

typedef struct reqresponse {
	uint64_t address;
	int pages;
	int page_migration;
	int status;

}REQRESPONSE;


// This defines a physical frame of size 4KB by default
class Frame{

	public:
		// Constructor
		Frame() { starting_address = 0; metadata = 0;}

		// Constructor with paramteres
		Frame(uint64_t st, uint64_t md) { starting_address = st; metadata = 0;}

		// The starting address of the frame
		uint64_t starting_address;

		// This will be used to store information about current allocation
		int metadata;

};


// This class defines a memory pool 

class Pool{

	public:

		//Constructor for pool
		Pool(SST::Component* own, Params parmas, SST::OpalComponent::MemType mem_type, int id);

		void finish() {}

		// The size of the memory pool in KBs
		uint32_t size;

		// The starting address of the memory pool
		uint64_t start;

		// Allocate N contigiuous frames, returns the starting address if successfull, or -1 if it fails!
		REQRESPONSE allocate_frame(int N);

		// Allocate 'size' contigiuous memory, returns a structure with starting address and number of frames allocated
		REQRESPONSE allocate_frames(int pages);

		// Freeing N frames starting from Address X, this will return -1 if we find that these frames were not allocated
		REQRESPONSE deallocate_frame(uint64_t X, int N);

		// Deallocate 'size' contigiuous memory starting from physical address 'starting_pAddress', returns a structure which indicates success or not
		REQRESPONSE deallocate_frames(int size, uint64_t starting_pAddress);

		// Current number of free frames
		int freeframes() { return freelist.size(); }

		// Frame size in KBs
		int frsize;

		//Total number of frames
		int num_frames;

		//real size of the memory pool
		uint32_t real_size;

		//number of free frames
		int available_frames;

		void set_memPool_type(SST::OpalComponent::MemType _memType) { memType = _memType; }

		SST::OpalComponent::MemType get_memPool_type() { return memType; }

		void set_memPool_tech(SST::OpalComponent::MemTech _memTech) { memTech = _memTech; }

		SST::OpalComponent::MemTech get_memPool_tech() { return memTech; }

		void setMemID(int id) { poolId = id; }

		int getMemID() { return poolId; }

		void build_mem();

		void profileStats(int stat, int value);

	private:

		SST::Component* owner;

		Output *output;

		//memory pool id
		int poolId;

		//shared or local
		SST::OpalComponent::MemType memType;

		//Memory technology
		SST::OpalComponent::MemTech memTech;

		// The list of free frames
		std::list<Frame*> freelist;

		// The list of allocated frames --- the key is the starting physical address
		std::map<uint64_t, Frame*> alloclist;

		Statistic<uint64_t>* memUsage;
		Statistic<uint64_t>* mappedMemory;
		Statistic<uint64_t>* unmappedMemory;
		Statistic<uint64_t>* tlbShootdowns;
		Statistic<uint64_t>* tlbShootdownDelay;

};

