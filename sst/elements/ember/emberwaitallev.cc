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
#include "emberwaitallev.h"

using namespace SST::Ember;

EmberWaitallEvent::EmberWaitallEvent(int cnt, MessageRequest reqs_p[],
                                                    bool delRequest) :
    reqs(reqs_p),
    delReq(delRequest),
    numMessageRequests(cnt) 
{
}

EmberWaitallEvent::~EmberWaitallEvent() {

}

MessageRequest* EmberWaitallEvent::getMessageRequestHandle() {
	return reqs;
}

int EmberWaitallEvent::getNumMessageRequests()
{
    return numMessageRequests;
}

EmberEventType EmberWaitallEvent::getEventType() {
	return WAITALL;
}

std::string EmberWaitallEvent::getPrintableString() {
	char buffer[128];
        sprintf(buffer, "Waitall Event");
        std::string bufferStr = buffer;
        return bufferStr;
}

bool EmberWaitallEvent::deleteRequestPointer() {
	return delReq;
}
