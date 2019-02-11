
#include <sst_config.h>

#include "shogunrrarb.h"
#include "shogun_event.h"

using namespace SST::Shogun;

ShogunRoundRobinArbitrator::ShogunRoundRobinArbitrator() :
	lastStart(0) {}

ShogunRoundRobinArbitrator::~ShogunRoundRobinArbitrator() {}

void ShogunRoundRobinArbitrator::moveEvents( const int port_count,
                ShogunQueue<ShogunEvent*>** inputQueues,
                ShogunEvent** outputEvents,
                uint64_t cycle ) {

	output->verbose(CALL_INFO, 4, 0, "BEGIN: Arbitration --------------------------------------------------\n");
	output->verbose(CALL_INFO, 4, 0, "-> lastStart %d\n", lastStart);

	int currentPort = lastStart;

	for( int i = 0; i < port_count; ++i ) {
		output->verbose(CALL_INFO, 4, 0, "-> process port %d, has-events? %s\n", currentPort,
			inputQueues[currentPort]->empty() ? "no" : "yes");

		if( inputQueues[currentPort]->empty() ) {

		} else {

		ShogunEvent* pendingEv = inputQueues[currentPort]->peek();

			output->verbose(CALL_INFO, 4, 0, "  -> attempting send to: %d, remote status: %s\n",
				pendingEv->getDestination(),
				outputEvents[pendingEv->getDestination()] == nullptr ? "empty" : "full");

		if( outputEvents[pendingEv->getDestination()] == nullptr ) {
			output->verbose(CALL_INFO, 4, 0, "  -> moving event to remote queue\n");
			pendingEv = inputQueues[currentPort]->pop();
			outputEvents[pendingEv->getDestination()] = pendingEv;
		}
		}

		// Increment to next port in sequence
		currentPort = nextPort(port_count, currentPort);
	}

	lastStart = nextPort(port_count, lastStart);

	output->verbose(CALL_INFO, 4, 0, "END: Arbitration ----------------------------------------------------\n");
}
