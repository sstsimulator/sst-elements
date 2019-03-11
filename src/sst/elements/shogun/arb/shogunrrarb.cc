
#include <sst_config.h>

#include "shogunrrarb.h"
#include "shogun_event.h"

using namespace SST::Shogun;

ShogunRoundRobinArbitrator::ShogunRoundRobinArbitrator() :
	lastStart(0) {}

ShogunRoundRobinArbitrator::~ShogunRoundRobinArbitrator() {}

void ShogunRoundRobinArbitrator::moveEvents( const int num_events, const int port_count,
                                             ShogunQueue<ShogunEvent*>** inputQueues,
                                             uint32_t output_slots,
                                             ShogunEvent*** outputEvents,
                                             uint64_t cycle ) {

    output->verbose(CALL_INFO, 4, 0, "BEGIN: Arbitration --------------------------------------------------\n");
	output->verbose(CALL_INFO, 4, 0, "-> start: %d\n", lastStart);

	int currentPort = lastStart;
	int moved_count = 0;

	for( int i = 0; i < port_count; ++i ) {
		auto nextQ = inputQueues[currentPort];
        output->verbose(CALL_INFO, 4, 0, "-> processing port: %d, event-count: %d out of %d\n", currentPort,
                        nextQ->count(), num_events);

        //Want to send num_events for each port
        for( auto j = 0; j < num_events; ++j ) {
            if( inputQueues[currentPort]->empty() ) {
                output->verbose(CALL_INFO, 4, 0, "  (%d)-> input queue empty...\n", j);
                break;
            } else {

                ShogunEvent* pendingEv = inputQueues[currentPort]->peek();

                for( auto k = 0; k < output_slots; ++k) {
                    output->verbose(CALL_INFO, 4, 0, "  (%d)-> attempting send from: %d to: %d, remote status: %s\n",
                        j, pendingEv->getSource(), pendingEv->getDestination(),
                        outputEvents[pendingEv->getDestination()][k] == nullptr ? "empty" : "full");

                    if( outputEvents[pendingEv->getDestination()][k] == nullptr ) {
                        output->verbose(CALL_INFO, 4, 0, "  (%d)-> moving event to remote queue\n");
                        j, pendingEv = inputQueues[currentPort]->pop();
                        outputEvents[pendingEv->getDestination()][k] = pendingEv;
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
