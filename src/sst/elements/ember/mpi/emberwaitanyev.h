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


#ifndef _H_EMBER_WAITANY_EV
#define _H_EMBER_WAITANY_EV

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberWaitAnyEvent : public EmberMPIEvent {

	public:
		EmberWaitAnyEvent(MP::Request* req, int* index,
			const int requestCount);
		EmberWaitAnyEvent(MessageRequest* req, MessageResponse* resp, int* index,
			const int requestCount);
		~EmberWaitAnyEvent();
		MessageRequest* getMessageRequestHandleArray();
		int getMessageRequestCount();
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
