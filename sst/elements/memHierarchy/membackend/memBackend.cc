//
//  memBackend.cpp
//  SST
//
//  Created by Caesar De la Paz III on 7/9/14.
//  Copyright (c) 2014 De la Paz, Cesar. All rights reserved.
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

MemBackend::MemBackend(Component *comp, Params &params) : Module(){
    ctrl = dynamic_cast<MemController*>(comp);
    if (!ctrl)
        _abort(MemBackend, "MemBackends expect to be loaded into MemControllers.\n");
}


