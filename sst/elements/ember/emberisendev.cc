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
#include "emberisendev.h"

using namespace SST::Ember;

EmberISendEvent::EmberISendEvent(uint32_t pRank, uint32_t pSendSizeBytes, int pTag, Communicator pComm, MessageRequest* req) {
	sendSizeBytes = pSendSizeBytes;
	rank = pRank;
	sendTag = pTag;
	comm = pComm;
	request = req;
}

EmberISendEvent::~EmberISendEvent() {

}

Communicator EmberISendEvent::getCommunicator() {
	return comm;
}

MessageRequest* EmberISendEvent::getMessageRequestHandle() {
	return request;
}

EmberEventType EmberISendEvent::getEventType() {
	return ISEND;
}

uint32_t EmberISendEvent::getSendToRank() {
	return rank;
}

uint32_t EmberISendEvent::getMessageSize() {
	return sendSizeBytes;
}

int EmberISendEvent::getTag() {
	return sendTag;
}

std::string EmberISendEvent::getPrintableString() {
	char buffer[128];
	sprintf(buffer, "ISend Event (Send to rank=%" PRIu32 ", Size=%" PRIu32 " bytes, tag=%d)",
		rank, sendSizeBytes, sendTag);
	std::string bufferStr = buffer;
	return bufferStr;
}
