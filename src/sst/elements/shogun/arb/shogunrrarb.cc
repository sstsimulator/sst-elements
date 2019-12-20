
#include <sst_config.h>

#include "shogun_event.h"
#include "shogunrrarb.h"
#include "shogun_stat_bundle.h"

using namespace SST::Shogun;

ShogunRoundRobinArbitrator::ShogunRoundRobinArbitrator()
    : lastStart(0)
{
}

ShogunRoundRobinArbitrator::~ShogunRoundRobinArbitrator() {}

void ShogunRoundRobinArbitrator::moveEvents(const int num_events,
                                            const int port_count,
                                            ShogunQueue<ShogunEvent*>** inputQueues,
                                            int32_t output_slots,
                                            ShogunEvent*** outputEvents,
                                            uint64_t cycle ) {

    output->verbose(CALL_INFO, 4, 0, "BEGIN: Arbitration --------------------------------------------------\n");
    output->verbose(CALL_INFO, 4, 0, "-> start: %" PRIi32 "\n", lastStart);

    int32_t currentPort = lastStart;
    int32_t moved_count = 0;

    // RR, so iterate through the ports one at a time and process num_events from the queue
    for (int32_t i = 0; i < port_count; ++i) {
        auto nextQ = inputQueues[currentPort];
        output->verbose(CALL_INFO, 4, 0, "-> processing port: %" PRIi32 ", event-count: %" PRIi32 " out of %" PRIi32 "\n", currentPort,
                        nextQ->count(), num_events);

        //Want to send num_events for each port
        int32_t j = 0;
        while (j < num_events || num_events == -1 ) {
            if (inputQueues[currentPort]->empty()) {
                output->verbose(CALL_INFO, 4, 0, "  (%" PRIi32 ")-> input queue empty...\n", j);
                break;
            } else {

                ShogunEvent* pendingEv = inputQueues[currentPort]->peek();

                int32_t k = 0;
                while (k < output_slots) {
                    output->verbose(CALL_INFO, 4, 0, "  (%" PRIi32 ")-> attempting send from: %" PRIi32 " to: %" PRIi32 ", remote status: %s\n",
                        j, pendingEv->getSource(), pendingEv->getDestination(),
                        outputEvents[pendingEv->getDestination()][k] == nullptr ? "empty" : "full");

                    if (outputEvents[pendingEv->getDestination()][k] == nullptr) {
                        output->verbose(CALL_INFO, 4, 0, "  (%" PRIi32 ")-> moving event to remote queue\n", j);
                        pendingEv = inputQueues[currentPort]->pop();
                        outputEvents[pendingEv->getDestination()][k] = pendingEv;
                        moved_count++;

                        break;
                    }

                    ++k;
                }

                if ( k == output_slots ) {
                   output->verbose(CALL_INFO, 4, 0, "  (%" PRIi32 ")-> output queue full...\n", j);
                   break;
                }
            }

            ++j;
        }
        // Increment to next port in sequence
        currentPort = nextPort(port_count, currentPort);
    }

    lastStart = nextPort(port_count, lastStart);

    bundle->getPacketsMoved()->addData(moved_count);
    output->verbose(CALL_INFO, 4, 0, "-> next-start: %" PRIi32 "\n", lastStart);
    output->verbose(CALL_INFO, 4, 0, "END: Arbitration ----------------------------------------------------\n");
}
