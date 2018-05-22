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


#ifndef _H_THORNHILL_MEMORY_HEAP_EVENT_EVENT
#define _H_THORNHILL_MEMORY_HEAP_EVENT_EVENT

#include <stdint.h>
#include <sst/core/event.h>
#include <sst/core/params.h>

#include "sst/elements/thornhill/types.h"

namespace SST {
namespace Thornhill {

class MemoryHeapEvent : public SST::Event {

public:
	typedef uint64_t Key;

	enum { Alloc, Free } type;

	Key 		key;
	size_t		length;
	SimVAddr    addr;

private:

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
		ser & type;
        ser & key;
        ser & length;
        ser & addr;
    }

    ImplementSerializable(SST::Thornhill::MemoryHeapEvent);
};
	
}
}

#endif
