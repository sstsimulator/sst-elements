
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

	int currentPort = lastStart;

	for( int i = 0; i < port_count; ++i ) {
		printf("Arbitrator: current-port: %d\n", currentPort);
		printf("-> pending events? %s\n", inputQueues[currentPort]->empty() ?
			"no" : "yes");

		if( inputQueues[currentPort]->empty() ) {

		} else {

		ShogunEvent* pendingEv = inputQueues[currentPort]->peek();

		printf("-> attempting to send to: %d\n", pendingEv->getDestination());
		printf("-> remote empty? %s\n", outputEvents[pendingEv->getDestination()] == nullptr ?
			"yes" : "no");

		if( outputEvents[pendingEv->getDestination()] == nullptr ) {
			printf("-> moving event, pop from queue\n");
			pendingEv = inputQueues[currentPort]->pop();
			outputEvents[pendingEv->getDestination()] = pendingEv;
		} else {
			printf("-> not moving event, remote dest is busy.\n");
		}
		}

		// Increment to next port in sequence
		currentPort = nextPort(port_count, currentPort);
	}

	lastStart = nextPort(port_count, lastStart);
}
