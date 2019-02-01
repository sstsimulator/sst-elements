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

#include <sst/core/event.h>
#include "shogun.h"

using namespace SST;
using namespace SST::Shogun;

ShogunComponent::ShogunComponent(ComponentId_t id, Params& params) : Component(id)
{
    const std::string clock_rate = params.find<std::string>("clock");

    master_port_count = params.find<int>("master_port_count");
    slave_port_count  = params.find<int>("slave_port_count");

    output->verbose(CALL_INFO, 1, 0, "Creating Shogun crossbar at %s clock rate and %d master ports and %d slave ports\n",
	clock_rate.c_str(), master_port_count, slave_port_count);

    registerClock(clock_rate, new Clock::Handler<ShogunComponent>(this,
                  &ShogunComponent::tick));

    if( master_port_count <= 0 || slave_port_count <= 0 ) {
	output->fatal(CALL_INFO, -1, "Error: you specified a master or slave port count of less than or equal to zero.\n" );
    }

    output->verbose(CALL_INFO, 1, 0, "Connecting %d master links...\n", master_port_count);
    links = (SST::Link**) malloc( sizeof(SST::Link*) * (master_port_count + slave_port_count) );
    char* linkName = new char[256];

    for( int i = 0; i < master_port_count; ++i ) {
	sprintf(linkName, "master_link%d", i);
	links[i] = configureLink(linkName);
	links[i]->setPolling();
    }

    output->verbose(CALL_INFO, 1, 0, "Connecting %d slave links...\n", slave_port_count);

    for( int i = 0; i < slave_port_count; ++i ) {
	sprintf(linkName, "slave_link%d", i);
	links[master_port_count + i] = configureLink(linkName);
	links[master_port_count + i]->setPolling();
    }

    delete[] linkName;

    output->verbose(CALL_INFO, 1, 0, "Allocating pending input/output slots...\n" );
    pendingInputs  = (ShogunEvent**) malloc( sizeof(ShogunEvent*) * (master_port_count + slave_port_count) );
    pendingOutputs = (ShogunEvent**) malloc( sizeof(ShogunEvent*) * (master_port_count + slave_port_count) );
}

ShogunComponent::~ShogunComponent()
{
    output->verbose(CALL_INFO, 1, 0, "Shogun destructor fired, closing down.\n");
    delete output;
}

ShogunComponent::ShogunComponent() : Component(-1)
{
    // for serialization only
}

bool ShogunComponent::tick( Cycle_t currentCycle )
{
    // Pull any pending events from incoming links
    populateInputs();

    // Migrate events across the cross-bar

    // Send any events which can be sent this cycle
    emitOutputs();

    // return false so we keep going
    return false;
}

void ShogunComponent::populateInputs() {
    output->verbose(CALL_INFO, 4, 0, "Processing input events...\n");
    int count = 0;

    for( int i = 0; i < (master_port_count + slave_port_count); ++i ) {
	if( nullptr == pendingInputs[i] ) {
		// Poll link for next event
		SST::Event* incoming = links[i]->recv();

		if( nullptr != incoming ) {
			ShogunEvent* incomingShogun = dynamic_cast<ShogunEvent*>(incoming);

			if( nullptr != incoming ) {
				pendingInputs[i] = incomingShogun;
				count++;
			} else {
				output->fatal(CALL_INFO, -1, "Error: received a non-shogun compatible event via a polling link (id=%d)\n", i);
			}
		}
	}
    }

    output->verbose(CALL_INFO, 4, 0, "Completed processing input events (%d new events)\n", count);
}

void ShogunComponent::emitOutputs() {
    for( int i = 0; i < (master_port_count + slave_port_count); ++i ) {
	if( nullptr != pendingOutputs[i] ) {
		links[i]->send( pendingOutputs[i] );
		pendingOutputs[i] = nullptr;
	}
    }
}

void ShogunComponent::clearOutputs() {
    for( int i = 0; i < (master_port_count + slave_port_count); ++i ) {
	pendingOutputs[i] = nullptr;
    }
}

void ShogunComponent::clearInputs() {
    for( int i = 0; i < (master_port_count + slave_port_count); ++i ) {
	pendingInputs[i] = nullptr;
    }
}
