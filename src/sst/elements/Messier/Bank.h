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

#ifndef _H_SST_NVM_BANK
#define _H_SST_NVM_BANK


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include<map>
#include<list>


// This class structure represents NVM memory Bank 
class BANK
{ 


	// This is the address of the last read address in that bank (timing differs if it hits in)
	long long int row_buff;

	// This determines if the row buffer has been written (important as it needs to be written back to NVM when evicted)
	bool row_buffer_dirty;
	
	// This determines the bank busy until time, this is used to enforce timing parameters like precharge and writeback
	long long BusyUntil;

	// This means the bank is locked until serving a specific request, this is important to avoid races in the time between activation ready and the controller read the data, the block must remain blocked until the data is read, and void being stolen by another request
	bool locked;	

	public: 

	BANK() { locked= false; row_buff = -1; row_buffer_dirty = false; BusyUntil = 0;}

	void setBusyUntil(long long int x) {BusyUntil = x;}
	long long int getBusyUntil() { return BusyUntil;}

	void setRB(long long int address) { row_buff = address;}
	long long int getRB() { return row_buff; }

	bool getLocked() { return locked;}
	void setLocked(bool x) { locked = x; }

};

#endif
