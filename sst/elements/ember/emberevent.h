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


#ifndef _H_EMBER_EVENT
#define _H_EMBER_EVENT

#include <sst/core/event.h>
#include <stdint.h>
#include <string>

namespace SST {
namespace Ember {

enum EmberDataType {
	EMBER_CHAR,
	EMBER_F64,
	EMBER_F32,
	EMBER_INT32,
	EMBER_INT64
};

enum EmberReductionOperation {
	EMBER_SUM
};

enum EmberEventType {
	INIT,
	FINALIZE,
	SEND,
	RECV,
	IRECV,
	ISEND,
	WAIT,
	WAITALL,
	COMPUTE,
	BARRIER,
	START,
	STOP,
	ALLREDUCE,
	REDUCE,
	GETTIME
};

class EmberEvent : public SST::Event {

public:
	EmberEvent();
	~EmberEvent();
	virtual EmberEventType getEventType() = 0;
	virtual std::string getPrintableString() = 0;

};

}
}

#endif
