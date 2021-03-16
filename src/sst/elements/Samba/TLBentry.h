// Copyright 2009-2021 NTESS. Under the terms
// // of Contract DE-NA0003525 with NTESS, the U.S.
// // Government retains certain rights in this software.
// //
// // Copyright (c) 2009-2021, NTESS
// // All rights reserved.
// //
// // This file is part of the SST software package. For license
// // information, see the LICENSE file in the top level directory of the
// // distribution.


// This defines the TLB entry
class TLBentry
{

	// Maintains the default page size in KB, default is 4
	int page_size;

	// Maintains permissions, not sure if we will end up using this later, for now: 0 means RWX, 1 means R-only
	bool permissions;

	// Maintains the virtual address, note that this is in the granularity of the page size of this entry
	uint64_t VA;

	// Maintains the physical address, note that this is in the granularity of the page size of this entry
	uint64_t PA;

	// Valid, indidicate if the entry is valid or not
	bool valid;

	public:

	// Constructor
	TLBentry(int PAGE_SIZE, bool PERMISSIONS){page_size=PAGE_SIZE; permissions=PERMISSIONS; VA=0; PA=0; valid=0;}

	void setEntry(uint64_t va, uint64_t pa, int PAGE_SIZE, bool PERMISSIONS){}

	bool IsValid(){return valid;}
	void Invalidate(){valid = false;}

	uint64_t getVA(){return VA;}
	uint64_t getPA(){return PA;}
	bool getPermission(){return permissions;}



};
