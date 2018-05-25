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


#ifndef _H_ZODIAC_RECV_EVENT_BASE
#define _H_ZODIAC_RECV_EVENT_BASE

#include "zevent.h"

using namespace SST::Hermes;
using namespace SST::Hermes::MP;

namespace SST {
namespace Zodiac {

class ZodiacRecvEvent : public ZodiacEvent {

	public:
		ZodiacRecvEvent(uint32_t src, uint32_t length, 
			PayloadDataType dataType,
			uint32_t tag, Communicator group);
		virtual ZodiacEventType getEventType();

		uint32_t getSource();
		uint32_t getLength();
		uint32_t getMessageTag();
		PayloadDataType getDataType();
		Communicator getCommunicatorGroup();

	protected:
		uint32_t msgSrc;
		uint32_t msgLength;
		uint32_t msgTag;
		PayloadDataType msgType;
		Communicator msgComm;

};

}
}

#endif
