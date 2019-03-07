
#include <sst_config.h>

#include "shogunrrarb.h"
#include "shogun_event.h"

using namespace SST::Shogun;

ShogunRoundRobinArbitrator::ShogunRoundRobinArbitrator() :
	lastStart(0) {}

ShogunRoundRobinArbitrator::~ShogunRoundRobinArbitrator() {}

void ShogunRoundRobinArbitrator::moveEvents( const int num_events, const int port_count,
                ShogunQueue<ShogunEvent*>** inputQueues,
                std::vector< std::vector< ShogunEvent* > >* outputEvents,
                uint64_t cycle ) {

	output->verbose(CALL_INFO, 4, 0, "BEGIN: Arbitration %d --------------------------------------------------\n", cycle);
	output->verbose(CALL_INFO, 4, 0, "-> start: %d\n", lastStart);

	int currentPort = lastStart;
	int moved_count = 0;

	for( int i = 0; i < port_count; ++i ) {
		auto nextQ = inputQueues[currentPort];
        output->verbose(CALL_INFO, 4, 0, "-> processing port: %d(%d), event-count: %d\n", currentPort, num_events,
			nextQ->count());

        //Want to send num_events for each port
        for( auto j = 0; j < num_events; ++j ) {
            if( inputQueues[currentPort]->empty() ) {
                //do nothing
            } else {

                ShogunEvent* pendingEv = inputQueues[currentPort]->peek();

                for( auto moop = 0; moop < 1; ++moop) {
                    output->verbose(CALL_INFO, 4, 0, "  (%d)-> attempting send from: %d to: %d, remote status: %s\n",
                        j, pendingEv->getSource(), pendingEv->getDestination(),
                        (*outputEvents)[pendingEv->getDestination()][moop] == nullptr ? "empty" : "full");

                    if( (*outputEvents)[pendingEv->getDestination()][moop] == nullptr ) {
                        output->verbose(CALL_INFO, 4, 0, "  (%d)-> moving event to remote queue\n");
                        j, pendingEv = inputQueues[currentPort]->pop();
                        (*outputEvents)[pendingEv->getDestination()][moop] = pendingEv;
                        moved_count++;

                        break;
                    }
                }
            }
        }
		// Increment to next port in sequence
		currentPort = nextPort(port_count, currentPort);
	}

	lastStart = nextPort(port_count, lastStart);

	bundle->getPacketsMoved()->addData(moved_count);
	output->verbose(CALL_INFO, 4, 0, "-> next-start: %d\n", lastStart);
	output->verbose(CALL_INFO, 4, 0, "END: Arbitration ----------------------------------------------------\n");
}
