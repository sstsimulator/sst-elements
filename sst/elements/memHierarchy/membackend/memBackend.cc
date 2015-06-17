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

//
//  memBackend.cpp
//  SST
//
//  Created by Caesar De la Paz III on 7/9/14.
//  Copyright (c) 2014-2015 De la Paz, Cesar. All rights reserved.
//

#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "memBackend.h"
#include "memoryController.h"
#include "util.h"

using namespace SST;
using namespace SST::MemHierarchy;

MemBackend::MemBackend(Component *comp, Params &params) : SubComponent(comp) {

    uint32_t verbose = (uint32_t) params.find_integer("verbose", 0);
    output = new SST::Output("MemoryBackend[@p:@l]: ", verbose, 0, SST::Output::STDOUT);

    ctrl = dynamic_cast<MemController*>(comp);
    if (!ctrl)
        output->fatal(CALL_INFO, -1, "MemBackends expect to be loaded into MemControllers.\n");
}

MemBackend::~MemBackend() {
	delete output;
}
