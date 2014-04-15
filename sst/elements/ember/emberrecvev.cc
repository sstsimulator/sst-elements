// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "emberrecvev.h"

using namespace SST::Ember;

EmberRecvEvent::EmberRecvEvent(uint32_t pRank, uint32_t pRecvSizeBytes, int pTag, Communicator pComm) {
	recvSizeBytes = pRecvSizeBytes;
	rank = pRank;
	recvTag = pTag;
	comm = pComm;
}

EmberRecvEvent::~EmberRecvEvent() {

}

Communicator EmberRecvEvent::getCommunicator() {
	return comm;
}

EmberEventType EmberRecvEvent::getEventType() {
	return RECV;
}

uint32_t EmberRecvEvent::getRecvFromRank() {
	return rank;
}

uint32_t EmberRecvEvent::getMessageSize() {
	return recvSizeBytes;
}

int EmberRecvEvent::getTag() {
	return recvTag;
}

std::string EmberRecvEvent::getPrintableString() {
	char buffer[128];
	sprintf(buffer, "Recv Event (Recv from rank=%" PRIu32 ", Size=%" PRIu32 " bytes, tag=%d)",
		rank, recvSizeBytes, recvTag);
	std::string bufferStr = buffer;
	return bufferStr;
}
