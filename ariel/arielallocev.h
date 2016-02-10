// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ARIEL_ALLOC_EVENT
#define _H_SST_ARIEL_ALLOC_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielAllocateEvent : public ArielEvent {

	public:
    		ArielAllocateEvent(uint64_t vAddr, uint64_t len, uint32_t lev, uint64_t ip);
		~ArielAllocateEvent();
		ArielEventType getEventType();
		uint64_t getVirtualAddress();
		uint64_t getAllocationLength();
		uint32_t getAllocationLevel();
    		uint64_t getInstructionPointer();

	protected:
		uint64_t virtualAddress;
		uint64_t allocateLength;
		uint32_t level;
    		uint64_t instPtr;

};

}
}

#endif
