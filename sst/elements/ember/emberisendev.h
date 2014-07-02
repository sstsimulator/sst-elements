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


#ifndef _H_EMBER_ISEND_EVENT
#define _H_EMBER_ISEND_EVENT

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberISendEvent : public EmberEvent {

public:
	EmberISendEvent(uint32_t rank, uint32_t sendSizeBytes, int tag, Communicator comm, MessageRequest* req);
	~EmberISendEvent();
	EmberEventType getEventType();
	uint32_t getSendToRank();
	uint32_t getMessageSize();
	MessageRequest* getMessageRequestHandle();
	int getTag();
	std::string getPrintableString();
	Communicator getCommunicator();

protected:
	MessageRequest* request;
	uint32_t sendSizeBytes;
	uint32_t rank;
	int sendTag;
	Communicator comm;

};

}
}

#endif
