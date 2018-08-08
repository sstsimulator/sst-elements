// Copyright 2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "STPU.h"

#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include "../memHierarchy/memEvent.h"

using namespace SST;
//using namespace SST::MemHierarchy;
using namespace SST::STPUComponent;

STPU::STPU(ComponentId_t id, Params& params) :
    Component(id)
{
    uint32_t outputLevel = params.find<uint32_t>("verbose", 0);
    out.init("STPU:@p:@l: ", outputLevel, 0, Output::STDOUT);

    // get parameters
    numNeurons = params.find<int>("neurons", 32);
    if (numNeurons <= 0) {
        out.fatal(CALL_INFO, -1,"number of neurons invalid\n");
    }    

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // init memory
    memory = dynamic_cast<Interfaces::SimpleMem*>(loadSubComponent("memHierarchy.memInterface", this, params));
    if (!memory) {
        out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.memInterface subcomponent\n");
    }
    memory->initialize("mem_link", new Interfaces::SimpleMem::Handler<STPU> (this, &STPU::handleEvent));

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<STPU>(this, &STPU::clockTic);
    clockTC = registerClock(clockFreq, clockHandler);
}

STPU::STPU() : Component(-1)
{
	// for serialization only
}


void STPU::init(unsigned int phase)
{
    memory->init(phase);
}

void STPU::handleEvent(Interfaces::SimpleMem::Request * req)
{
    std::map<uint64_t, SimTime_t>::iterator i = requests.find(req->id);
    if (i == requests.end()) {
	out.fatal(CALL_INFO, -1, "Request ID (%" PRIx64 ") not found in outstanding requests!\n", req->id);
    } else {
        requests.erase(i);
        // handle event
    }
    delete req;
}

bool STPU::clockTic( Cycle_t )
{
    // return false so we keep going
    return false;
}


