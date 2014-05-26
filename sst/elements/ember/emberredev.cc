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
#include "emberredev.h"

using namespace SST::Ember;

EmberReduceEvent::EmberReduceEvent(uint32_t elementCount, EmberDataType elementType,
	EmberReductionOperation op, uint32_t root, Communicator communicator) :

	redRoot(root),
	comm(communicator),
	elemCount(elementCount),
	elemType(elementType),
	reduceOp(op)
{

}

uint32_t EmberReduceEvent::getReductionRoot() {
	return redRoot;
}

EmberReduceEvent::~EmberReduceEvent() {

}

uint32_t EmberReduceEvent::getElementCount() {
	return elemCount;
}

EmberDataType EmberReduceEvent::getElementType() {
	return elemType;
}

EmberReductionOperation EmberReduceEvent::getReductionOperation() {
	return reduceOp;
}

Communicator EmberReduceEvent::getCommunicator() {
	return comm;
}

EmberEventType EmberReduceEvent::getEventType() {
	return REDUCE;
}

std::string EmberReduceEvent::getPrintableString() {
	return "Reduce event";
}
