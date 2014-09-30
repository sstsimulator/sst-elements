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
#include <stdlib.h>
#include "emberwaitanyev.h"

using namespace SST::Ember;

EmberWaitAnyEvent::EmberWaitAnyEvent(MessageRequest* req, int* indx,
                        const int requestCount) :
	request(req),
	count(requestCount),
	index(indx) {

	response = NULL;
}

EmberWaitAnyEvent::EmberWaitAnyEvent(MessageRequest* req,
			MessageResponse* resp,
			int* indx,
                        const int requestCount) :
	request(req),
	response(resp),
	count(requestCount),
	index(indx) {
}

EmberWaitAnyEvent::~EmberWaitAnyEvent() {
}

MessageRequest* EmberWaitAnyEvent::getMessageRequestHandleArray() {
	return request;
}

int EmberWaitAnyEvent::getMessageRequestCount() {
	return count;
}

EmberEventType EmberWaitAnyEvent::getEventType() {
	return WAITANY;
}

std::string EmberWaitAnyEvent::getPrintableString() {
	char* buffer = (char*) malloc(sizeof(char) * 1024);
	sprintf(buffer, "Wait Any Event, contains %d requests for processing", count);
	std::string bufStr(buffer);

	return bufStr;
}

bool EmberWaitAnyEvent::requestsMessageResponse() {
	return (response != NULL);
}
