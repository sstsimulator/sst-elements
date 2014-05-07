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
#include "emberallredev.h"

using namespace SST::Ember;

EmberAllreduceEvent::EmberAllreduceEvent(uint32_t elementCount, EmberDataType elementType,
	EmberReductionOperation op, Communicator communicator) :

	comm(communicator),
	elemCount(elementCount),
	elemType(elementType),
	reduceOp(op)
{

}

EmberAllreduceEvent::~EmberAllreduceEvent() {

}

uint32_t EmberAllreduceEvent::getElementCount() {
	return elemCount;
}

EmberDataType EmberAllreduceEvent::getElementType() {
	return elemType;
}

EmberReductionOperation EmberAllreduceEvent::getReductionOperation() {
	return reduceOp;
}

Communicator EmberAllreduceEvent::getCommunicator() {
	return comm;
}

EmberEventType EmberAllreduceEvent::getEventType() {
	return ALLREDUCE;
}

std::string EmberAllreduceEvent::getPrintableString() {
	return "Allreduce event";
}
