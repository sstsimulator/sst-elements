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
#include "zallredevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacAllreduceEvent::ZodiacAllreduceEvent(
	uint32_t length,
	PayloadDataType dataType,
	ReductionOperation op,
	Communicator group) {

	msgLength = length;
	msgType = dataType;
	mpiOp = op;
	msgComm = group;
}


ZodiacEventType ZodiacAllreduceEvent::getEventType() {
	return Z_ALLREDUCE;
}

ReductionOperation ZodiacAllreduceEvent::getOp() {
	return mpiOp;
}

uint32_t ZodiacAllreduceEvent::getLength() {
	return msgLength;
}

PayloadDataType ZodiacAllreduceEvent::getDataType() {
	return msgType;
}

Communicator ZodiacAllreduceEvent::getCommunicatorGroup() {
	return msgComm;
}
