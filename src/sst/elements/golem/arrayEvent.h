// Copyright 2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _ARRAYEVENT_H
#define _ARRAYEVENT_H

#include <sst/core/event.h>

namespace SST {
namespace Golem {


class ArrayEvent : public SST::Event {
public:
	ArrayEvent() {}; //For serialization only
	ArrayEvent(uint32_t array) : SST::Event(), arrayID(array) {}

	uint32_t getArrayID() { return arrayID; };

protected:
	uint32_t arrayID;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
	    Event::serialize_order(ser);
	    ser & arrayID;
	}
	ImplementSerializable(SST::Golem::ArrayEvent);
};

}
}
#endif /* _ARRAYEVENT_H */