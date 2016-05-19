// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
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
	std::string generator;
	SST::Params params;
	uint64_t 	key;
private:
	NotSerializable(MirandaReqEvent)	
};

class MirandaRspEvent : public SST::Event {
public:
	uint64_t 	key;
private:
	NotSerializable(MirandaRspEvent)	
};


}
}

#endif
