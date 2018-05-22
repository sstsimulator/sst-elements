// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "zsendevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacSendEvent::ZodiacSendEvent(
	uint32_t dest,
	uint32_t length,
        PayloadDataType dataType,
        uint32_t tag,
	Communicator group) {

	msgDest = dest;
	msgLength = length;
	msgTag = tag;
	msgType = dataType;
	msgComm = group;
}

ZodiacEventType ZodiacSendEvent::getEventType() {
	return Z_SEND;
}

uint32_t ZodiacSendEvent::getDestination() {
	return msgDest;
}

uint32_t ZodiacSendEvent::getLength() {
	return msgLength;
}

uint32_t ZodiacSendEvent::getMessageTag() {
	return msgTag;
}

PayloadDataType ZodiacSendEvent::getDataType() {
	return msgType;
}

Communicator ZodiacSendEvent::getCommunicatorGroup() {
	return msgComm;
}
