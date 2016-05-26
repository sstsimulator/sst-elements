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


#ifndef _H_SST_SAVANNAH_EVENT
#define _H_SST_SAVANNAH_EVENT

#include "sst/elements/memHierarchy/memoryController.h"
#include "sst/elements/memHierarchy/membackend/memBackend.h"
#include "sst/elements/memHierarchy/DRAMReq.h"

using namespace SST::MemHierarchy;

namespace SST {
namespace Savannah {

class SavannahRequestEvent : public SST::Event {
public:
	SavannahRequestEvent(DRAMReq& req) :
		request(req) {

		recvLink = 0;
	};

	DRAMReq& getRequest() {
		return request;
	}

	DRAMReq* getRequestPtr() {
		return &request;
	}

	uint32_t getLink() const {
		return recvLink;
	}

	void setLink(const uint32_t linkID) {
		recvLink = linkID;
	}

private:
	DRAMReq request;
	uint32_t recvLink;
};

}
}

#endif
