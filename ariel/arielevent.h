// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ARIEL_EVENT
#define _H_SST_ARIEL_EVENT

#include <sst/core/serialization.h>


namespace SST {
namespace ArielComponent {

enum ArielEventType {
	READ_ADDRESS,
	WRITE_ADDRESS,
	START_DMA_TRANSFER,
	WAIT_ON_DMA_TRANSFER,
	CORE_EXIT,
	NOOP,
	MALLOC,
	FREE,
	SWITCH_POOL
};

class ArielEvent {

	public:
		ArielEvent();
		virtual ~ArielEvent();
		virtual ArielEventType getEventType() const = 0;

};

}
}

#endif
