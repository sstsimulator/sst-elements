// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"

#include "sst/elements/carcosa/components/sensorEvent.h"
#include "sst/elements/carcosa/components/sensorComponent.h"


using namespace SST;
using namespace SST::Carcosa;

SensorComponent::SensorComponent(ComponentId_t id, Params& params) : Component(id)
{
    requireLibrary("memHierarchy");

    out = new Output("", 1, 0, Output::STDOUT);

    bool found;
    eventsToSend = params.find<unsigned>("eventsToSend", 0, found);
    if (!found) {
        out->fatal(CALL_INFO, -1, "Error in %s: the input did not specify 'eventsToSend' parameter\n", getName().c_str());
    }

    verbose = params.find<bool>("verbose", false);

    iflLink = configureLink("ifl", new Event::Handler<SensorComponent, &SensorComponent::handleSensorEvent>(this));

    registerClock("1Hz", new Clock::Handler<SensorComponent, &SensorComponent::mainTick>(this));
    sst_assert(iflLink, CALL_INFO, -1, "Error in %s: IFL link configuration failed\n", getName().c_str());

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    eventsReceived = 0;
    eventsForwarded = 0;
    eventsSent = 0;
}

// Announces this sensor to the IFL and drains any incoming init data.
void SensorComponent::init(unsigned phase)
{
    if (phase == 0) {
        SensorEvent* event = new SensorEvent(getName());
        iflLink->sendUntimedData(event);
    }

    while (SST::Event* ev = iflLink->recvUntimedData()) {

        SensorEvent* event = dynamic_cast<SensorEvent*>(ev);
        if (event) {
                if (verbose) out->output("    %" PRIu64 " %s received %s\n", getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
                delete event;
        } else {
            out->fatal(CALL_INFO, -1, "Error in %s: Received an event during init() but it is not the expected type\n", getName().c_str());
        }
    }
}

void SensorComponent::setup()
{
    if (iflLink == NULL) {
        primaryComponentOKToEndSim();
        return;
    } else if (eventsToSend == 0) {
        primaryComponentOKToEndSim();
        return;
    }

    iflLink->send(new SensorEvent("hi"));
    eventsSent++;
}


// IFL shouldn't normally send events back to the sensor; treat as unexpected.
void SensorComponent::handleSensorEvent(SST::Event *ev)
{
    SensorEvent *event = dynamic_cast<SensorEvent*>(ev);

    if (event) {
        if (verbose) out->output("    %" PRIu64 " %s received %s\n", getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
    } else {
        out->fatal(CALL_INFO, -1, "Error in %s: Received an event during simulation but it is not the expected type\n", getName().c_str());
    }
}

void SensorComponent::complete(unsigned phase)
{
    if (phase == 0) {
        std::string goodbye = "Goodbye from " + getName();
        iflLink->sendUntimedData( new SensorEvent(goodbye) );
    }

    while (SST::Event* ev = iflLink->recvUntimedData()) {
        SensorEvent* event = dynamic_cast<SensorEvent*>(ev);
        if (event) {
            if (verbose) out->output("    %" PRIu64 " %s received %s\n", getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
            iflMsg = event->getStr();
            delete event;
        } else {
            out->fatal(CALL_INFO, -1, "Error in %s: Received an event during complete() but it is not the expected type\n", getName().c_str());
        }
    }

}

void SensorComponent::finish()
{
}

SensorComponent::~SensorComponent()
{
    delete out;
}

void SensorComponent::emergencyShutdown() {
    out->output("Uh-oh, my name is %s and I have to quit. I sent %u messages.\n", getName().c_str(), eventsSent);
}

void SensorComponent::printStatus(Output& sim_out) {
    sim_out.output("%s reporting. I have sent %u messages, received %u, and forwarded %u.\n",
            getName().c_str(), eventsSent, eventsReceived, eventsForwarded);
}
bool SensorComponent::mainTick( Cycle_t cycles)
{
  iflLink->send(new SensorEvent("tick"));
  eventsSent++;
  if (eventsSent >= eventsToSend) {
      SensorEvent *ev = new SensorEvent("end");
      ev->setLast();
      iflLink->send(ev);
      primaryComponentOKToEndSim();
      return true;
  } else {
      return false;
  }
}
