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


#ifndef _H_EMBER_WAITANY_EV
#define _H_EMBER_WAITANY_EV

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberWaitAnyEvent : public EmberEvent {

	public:
		EmberWaitAnyEvent(MessageRequest* req, int* index,
			const int requestCount);
		EmberWaitAnyEvent(MessageRequest* req, MessageResponse* resp, int* index,
			const int requestCount);
		~EmberWaitAnyEvent();
		MessageRequest* getMessageRequestHandleArray();
		int getMessageRequestCount();
		EmberEventType getEventType();
		std::string getPrintableString();
		bool requestsMessageResponse();

	private:
		MessageRequest* request;
		MessageResponse* response;
		int count;
		int* index;

};

}
}

#endif
