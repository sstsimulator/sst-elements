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


#include <sst_config.h>
#include "zrecvevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacRecvEvent::ZodiacRecvEvent(uint32_t src, uint32_t length,
                        PayloadDataType dataType,
                        uint32_t tag, Communicator group) {

	msgSrc = src;
	msgLength = length;
	msgTag = tag;
	msgType = dataType;
	msgComm = group;
}

ZodiacEventType ZodiacRecvEvent::getEventType() {
	return Z_RECV;
}

uint32_t ZodiacRecvEvent::getSource() {
	return msgSrc;
}

uint32_t ZodiacRecvEvent::getLength() {
	return msgLength;
}

uint32_t ZodiacRecvEvent::getMessageTag() {
	return msgTag;
}

PayloadDataType ZodiacRecvEvent::getDataType() {
	return msgType;
}

Communicator ZodiacRecvEvent::getCommunicatorGroup() {
	return msgComm;
}
