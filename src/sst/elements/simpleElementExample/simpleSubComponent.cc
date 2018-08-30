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

#include "sst_config.h"

#include <sst/core/output.h>
#include <sst/core/clock.h>

#include "simpleSubComponent.h"
#include "simpleMessage.h"

using namespace SST;
using namespace SST::SimpleSubComponent;


SubComponentLoader::SubComponentLoader(ComponentId_t id, Params &params) :
    Component(id)
{
    SubComponentSlotInfo* info = getSubComponentSlotInfo("mySubComp");
    if ( !info ) {
        Output::getDefaultObject().fatal(CALL_INFO, -1, "Must specify at least one SubComponent for slot mySubComp.\n");
    }

    info->createAll(subComps);
    
    std::string freq = params.find<std::string>("clock", "1GHz");
    registerClock( freq,
        new Clock::Handler<SubComponentLoader>(this, &SubComponentLoader::tick ));

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}


bool SubComponentLoader::tick(Cycle_t cyc)
{
    for ( auto sub : subComps ) {
        sub->clock(cyc);
    }
    return false;
}




SubCompSender::SubCompSender(Component *owningComponent, Params &params) :
    SubCompInterface(owningComponent)
{
    nToSend = params.find<uint32_t>("sendCount", 10);

    nMsgSent = registerStatistic<uint32_t>("numSent", "");

    registerTimeBase("1GHz", true);

    link = configureLink("sendPort");
    if ( !link ) {
        Output::getDefaultObject().fatal(CALL_INFO, -1,
                "Failed to configure port 'sendPort'\n");
    }
}

void SubCompSender::clock(Cycle_t cyc)
{
    if ( nToSend == 0 )
        return;

    if ( (cyc % 64) == 0 ) {
        link->send(new SimpleMessageGeneratorComponent::simpleMessage());
        nMsgSent->addData(1);
        nToSend--;
    }
}



SubCompReceiver::SubCompReceiver(Component *owningComponent, Params &params) :
    SubCompInterface(owningComponent)
{
    nMsgReceived = registerStatistic<uint32_t>("numRecv", "");
    link = configureLink("recvPort",
            new Event::Handler<SubCompReceiver>(this, &SubCompReceiver::handleEvent));
    if ( !link ) {
        Output::getDefaultObject().fatal(CALL_INFO, -1,
                "Failed to configure port 'recvPort'\n");
    }
    registerTimeBase("1GHz", true);
}

void SubCompReceiver::clock(Cycle_t cyc)
{
    /* Do nothing */
}

void SubCompReceiver::handleEvent(Event *ev)
{
    nMsgReceived->addData(1);
}
