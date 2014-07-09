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
#include "emberwaitev.h"

using namespace SST::Ember;

EmberWaitEvent::EmberWaitEvent(MessageRequest* req, bool delRequest) :
	delReq(delRequest) {

	request = req;
}

EmberWaitEvent::~EmberWaitEvent() {

}

MessageRequest* EmberWaitEvent::getMessageRequestHandle() {
	return request;
}

EmberEventType EmberWaitEvent::getEventType() {
	return WAIT;
}

std::string EmberWaitEvent::getPrintableString() {
	char buffer[128];
        sprintf(buffer, "Wait Event");
        std::string bufferStr = buffer;
        return bufferStr;
}

bool EmberWaitEvent::deleteRequestPointer() {
	return delReq;
}
