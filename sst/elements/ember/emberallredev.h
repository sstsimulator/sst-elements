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


#ifndef _H_EMBER_ALLREDUCE_EV
#define _H_EMBER_ALLREDUCE_EV

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberAllreduceEvent : public EmberEvent {
	public:
		EmberAllreduceEvent(uint32_t elementCount, EmberDataType elementType, EmberReductionOperation op, Communicator comm);
		~EmberAllreduceEvent();

		uint32_t getElementCount();
		EmberDataType getElementType();
		EmberReductionOperation getReductionOperation();
		Communicator getCommunicator();

		EmberEventType getEventType();
	        std::string getPrintableString();

	private:
		Communicator comm;
		uint32_t elemCount;
		EmberDataType elemType;
		EmberReductionOperation reduceOp;

};

}
}

#endif
