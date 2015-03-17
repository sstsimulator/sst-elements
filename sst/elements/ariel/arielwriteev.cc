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
#include "arielwriteev.h"

using namespace SST::ArielComponent;

ArielWriteEvent::ArielWriteEvent(uint64_t wAddr, uint32_t len) {
	writeAddress = wAddr;
	writeLength = len;
}

ArielWriteEvent::~ArielWriteEvent() {

}

ArielEventType ArielWriteEvent::getEventType() {
	return WRITE_ADDRESS;
}

uint64_t ArielWriteEvent::getAddress() {
	return writeAddress;
}

uint32_t ArielWriteEvent::getLength() {
	return writeLength;
}
