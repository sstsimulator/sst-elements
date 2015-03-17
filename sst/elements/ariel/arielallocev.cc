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


#include <sst_config.h>
#include "arielallocev.h"

using namespace SST::ArielComponent;

ArielAllocateEvent::ArielAllocateEvent(uint64_t vAddr, uint64_t allocLen, uint32_t lev) {
	virtualAddress = vAddr;
	allocateLength = allocLen;
	level = lev;
}

ArielAllocateEvent::~ArielAllocateEvent() {

}

ArielEventType ArielAllocateEvent::getEventType() {
	return MALLOC;
}

uint64_t ArielAllocateEvent::getVirtualAddress() {
	return virtualAddress;
}

uint64_t ArielAllocateEvent::getAllocationLength() {
	return allocateLength;
}

uint32_t ArielAllocateEvent::getAllocationLevel() {
	return level;
}
