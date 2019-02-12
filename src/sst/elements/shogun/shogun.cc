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
#include <sst/core/event.h>

#include "shogun.h"
#include "arb/shogunrrarb.h"
#include "shogun_init_event.h"
#include "shogun_credit_event.h"

using namespace SST;
using namespace SST::Shogun;

ShogunComponent::ShogunComponent(ComponentId_t id, Params& params) : Component(id)
{
    const std::string clock_rate = params.find<std::string>("clock", "1.0GHz");
    queue_slots = 64;
    arb = new ShogunRoundRobinArbitrator();

    char prefix[256];
    sprintf(prefix, "[t=@t][%s]: ", getName().c_str());
    output = new SST::Output( prefix, 16, 0, Output::STDOUT );
    arb->setOutput(output);

    port_count = params.find<int>("port_count");

    output->verbose(CALL_INFO, 1, 0, "Creating Shogun crossbar at %s clock rate and %d ports\n",
	clock_rate.c_str(), port_count);

    registerClock(clock_rate, new Clock::Handler<ShogunComponent>(this,
                  &ShogunComponent::tick));

    if( port_count <= 0 ) {
	output->fatal(CALL_INFO, -1, "Error: you specified a port count of less than or equal to zero.\n" );
    }

    output->verbose(CALL_INFO, 1, 0, "Connecting %d links...\n", port_count);
    links = (SST::Link**) malloc( sizeof(SST::Link*) * (port_count) );
    char* linkName = new char[256];

    for( int i = 0; i < port_count; ++i ) {
	sprintf(linkName, "port%d", i);
	output->verbose(CALL_INFO, 1, 0, "Configuring port %s ...\n", linkName);

	links[i] = configureLink(linkName);

	if( nullptr == links[i] ) {
		output->fatal(CALL_INFO, -1, "Failed to configure link on port %d\n", i);
	}

	links[i]->setPolling();
    }

    delete[] linkName;

    output->verbose(CALL_INFO, 1, 0, "Allocating pending input/output queues...\n" );
    inputQueues = (ShogunQueue<ShogunEvent*>**) malloc( sizeof(ShogunQueue<ShogunEvent*>*) * port_count );
    pendingOutputs = (ShogunEvent**) malloc( sizeof(ShogunEvent*) * (port_count) );
    remote_output_slots = (int*) malloc( sizeof(int) * port_count );

    for( int i = 0; i < port_count; ++i ) {
	inputQueues[i] = new ShogunQueue<ShogunEvent*>( queue_slots );
	remote_output_slots[i] = 2;
    }

    clearOutputs();
}

ShogunComponent::~ShogunComponent()
{
    output->verbose(CALL_INFO, 1, 0, "Shogun destructor fired, closing down.\n");
    delete output;
    delete arb;
}

ShogunComponent::ShogunComponent() : Component(-1)
{
    // for serialization only
}

bool ShogunComponent::tick( Cycle_t currentCycle )
{
    output->verbose(CALL_INFO, 4, 0, "TICK() START *****************************************************\n");
    printStatus();

    // Pull any pending events from incoming links
    populateInputs();

    // Migrate events across the cross-bar
    arb->moveEvents( port_count, inputQueues, pendingOutputs, static_cast<uint64_t>( currentCycle ) );

    printStatus();

    // Send any events which can be sent this cycle
    emitOutputs();

    printStatus();
    output->verbose(CALL_INFO, 4, 0, "TICK() END *******************************************************\n");

    // return false so we keep going
    return false;
}

void ShogunComponent::init(unsigned int phase) {
	output->verbose(CALL_INFO, 2, 0, "Executing initialization phase %u...\n", phase);

	if( 0 == phase ) {
		for( int i = 0; i < port_count; ++i ) {
			links[i]->sendUntimedData( new ShogunInitEvent( port_count, i, inputQueues[i]->capacity() ) );
		}
	}

	for( int i = 0; i < port_count; ++i ) {
		SST::Event* ev = links[i]->recvUntimedData();

		while( nullptr != ev ) {

			if( nullptr != ev ) {
				ShogunInitEvent* initEv     = dynamic_cast<ShogunInitEvent*>(ev);
				ShogunCreditEvent* creditEv = dynamic_cast<ShogunCreditEvent*>(ev);

				// if the event is not a ShogunInit then we want to broadcast it out
				// if it is an init event, just drop it
				if( ( nullptr == initEv ) && ( nullptr == creditEv )) {

					for( int j = 0; j < port_count; ++j ) {
						if( i != j ) {
							printf("sending untimed data from %d to %d\n", i, j);
							links[j]->sendUntimedData( ev->clone() );
						}
					}
				} else {
					delete ev;
				}
			}

			ev = links[i]->recvUntimedData();
		}
	}
}

void ShogunComponent::populateInputs() {
    output->verbose(CALL_INFO, 4, 0, "BEGIN: processing x-bar inputs -----------------------------------------------\n");
    output->verbose(CALL_INFO, 4, 0, "Port Status:\n");
    for( int i = 0; i < port_count; ++i ) {
    output->verbose(CALL_INFO, 4, 0, "port %5d / in-q-count: %5d / remote-losts: %5d\n", i, inputQueues[i]->count(), remote_output_slots[i]);
    }
    output->verbose(CALL_INFO, 4, 0, "Processing inputs...\n");

    int count = 0;

    for( int i = 0; i < port_count; ++i ) {
	output->verbose(CALL_INFO, 4, 0, "-> processing port %d input-queue-count: %d\n", i, inputQueues[i]->count());

	if( ! inputQueues[i]->full() ) {
		// Poll link for next event
		SST::Event* incoming = links[i]->recv();

		if( nullptr != incoming ) {
			ShogunEvent* incomingShogun = dynamic_cast<ShogunEvent*>(incoming);

			if( nullptr != incomingShogun ) {
				output->verbose(CALL_INFO, 4, 0, "  -> recv from: %d dest: %d\n",
					incomingShogun->getPayload()->src,
					incomingShogun->getPayload()->dest);

				inputQueues[i]->push( incomingShogun );
				count++;
			} else {
				ShogunCreditEvent* creditEv = dynamic_cast<ShogunCreditEvent*>( incoming );

				if( nullptr != creditEv ) {
					output->verbose(CALL_INFO, 8, 0, "-> recv credit event, output-slots on %d now %d\n", i, remote_output_slots[i]);
					remote_output_slots[i]++;
				} else {
					output->fatal(CALL_INFO, -1, "Error: received a non-shogun compatible event via a polling link (id=%d)\n", i);
				}
			}
		}
	} else {
		output->verbose(CALL_INFO, 4, 0, "-> queues for port %d are full, cannot accept any events\n");
	}
    }

    output->verbose(CALL_INFO, 4, 0, "END: processing x-bar inputs -------------------------------------------------\n");
}

void ShogunComponent::emitOutputs() {
    output->verbose(CALL_INFO, 4, 0, "BEGIN: emitOutputs -----------------------------------------------\n");

    for( int i = 0; i < port_count; ++i ) {
	output->verbose(CALL_INFO, 4, 0, "-> Processing port %d:\n", i);

	if( nullptr != pendingOutputs[i] ) {
		output->verbose(CALL_INFO, 4, 0, "  -> output is not null, remote-slot-count: %d, src=%5d\n", remote_output_slots[i],
			pendingOutputs[i]->getSource());

		if( ( remote_output_slots[i] > 0 ) ) {
			output->verbose(CALL_INFO, 4, 0, "    -> sending event (has entry and free %d slots)\n", remote_output_slots[i]);

			links[i]->send( pendingOutputs[i] );
			links[ pendingOutputs[i]->getSource() ]->send( new ShogunCreditEvent() );
			pendingOutputs[i] = nullptr;
			remote_output_slots[i]--;
		} else {
			output->verbose(CALL_INFO, 4, 0, "    -> no free slots, event send disabled for this round (slots: %d)\n", remote_output_slots[i]);
		}
	}
    }

    output->verbose(CALL_INFO, 4, 0, "END: emitOutputs -------------------------------------------------\n");
}

void ShogunComponent::clearOutputs() {
    for( int i = 0; i < port_count; ++i ) {
	pendingOutputs[i] = nullptr;
	remote_output_slots[i] = inputQueues[i]->capacity();
    }
}

void ShogunComponent::clearInputs() {
    for( int i = 0; i < port_count; ++i ) {
	inputQueues[i]->clear();
    }
}

void ShogunComponent::printStatus() {
    output->verbose(CALL_INFO, 4, 0, "BEGIN: processing x-bar inputs -----------------------------------------------\n");
    output->verbose(CALL_INFO, 4, 0, "BEGIN X-BAR STATUS REPORT ====================================================\n");
    for( int i = 0; i < port_count; ++i ) {
    output->verbose(CALL_INFO, 4, 0, "port %5d / in-q-count: %5d / remote-losts: %5d / out-q: %s\n", i,
	inputQueues[i]->count(), remote_output_slots[i], pendingOutputs[i] == nullptr ? "empty" : "full");
    }
    output->verbose(CALL_INFO, 4, 0, "END X-BAR STATUS REPORT ======================================================\n");
}
