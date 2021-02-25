// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEM_H_REQUEST_GEN_EVENT
#define _H_SST_MEM_H_REQUEST_GEN_EVENT

#include <stdint.h>
#include <sst/core/event.h>
#include <sst/core/params.h>

namespace SST {
namespace Miranda {

class MirandaReqEvent : public SST::Event {
public:
    struct Generator {
        std::string name;
        SST::Params params;
    };

	std::deque< std::pair< std::string, SST::Params> > generators;

	uint64_t 	key;
private:

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & key;
		ser & generators;
    }

    ImplementSerializable(SST::Miranda::MirandaReqEvent);
};

class MirandaRspEvent : public SST::Event {
public:
	uint64_t 	key;
private:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
		ser & key;
	}
    ImplementSerializable(SST::Miranda::MirandaRspEvent);
};


}
}

#endif
