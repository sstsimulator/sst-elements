// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_ZODIAC_ALLREDUCE_EVENTBASE
#define _H_ZODIAC_ALLREDUCE_EVENTBASE

#include "zcollective.h"

using namespace SST::Hermes;
using namespace SST::Hermes::MP;

namespace SST {
namespace Zodiac {

class ZodiacAllreduceEvent : public ZodiacCollectiveEvent {

	public:
		ZodiacAllreduceEvent(uint32_t length,
			PayloadDataType dataType,
			ReductionOperation op, Communicator group);
		ZodiacEventType getEventType();

		uint32_t getLength();
		PayloadDataType getDataType();
		Communicator getCommunicatorGroup();
		ReductionOperation getOp();

	private:
		uint32_t msgLength;
		PayloadDataType msgType;
		Communicator msgComm;
		ReductionOperation mpiOp;

};

}
}

#endif
