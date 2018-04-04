// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ARIEL_FREE_EVENT
#define _H_SST_ARIEL_FREE_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielFenceEvent : public ArielEvent {

	public:
		ArielEventType getEventType() const { return FENCE; }
		uint64_t getAddress() const { return address; }
		uint64_t getLength() const { return length; }

	protected:
		uint64_t address;
		uint64_t length;

};

}
}

#endif
