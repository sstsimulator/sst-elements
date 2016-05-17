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


#ifndef __SST_MEMH_BACKEND__
#define __SST_MEMH_BACKEND__

#include <sst/core/event.h>
#include <sst/core/output.h>

#include <iostream>
#include <map>

#include "sst/elements/memHierarchy/memoryController.h"
#include "sst/elements/memHierarchy/DRAMReq.h"

#define NO_STRING_DEFINED "N/A"

namespace SST {
namespace MemHierarchy {

class MemController;

class MemBackend : public SubComponent {
public:
    MemBackend();

    MemBackend(Component *comp, Params &params) :
	SubComponent(comp) {

	uint32_t verbose = params.find<uint32_t>("verbose", 0);
    	output = new SST::Output("MemoryBackend[@p:@l]: ", verbose, 0, SST::Output::STDOUT);

    	ctrl = dynamic_cast<MemController*>(comp);
    	if (!ctrl) {
        	output->fatal(CALL_INFO, -1, "MemBackends expect to be loaded into MemControllers.\n");
	}
    }

    virtual ~MemBackend() {
	delete output;
    }

    virtual bool issueRequest(DRAMReq *req) = 0;
    virtual void setup() {}
    virtual void finish() {}
    virtual void clock() {}
protected:
    MemController *ctrl;
    Output* output;

};

}}

#endif

