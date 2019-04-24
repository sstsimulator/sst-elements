
#ifndef _H_SHOGUN_RANDOM_ARB_H
#define _H_SHOGUN_RANDOM_ARB_H

#include "shogun_event.h"
#include "shogunarb.h"

namespace SST {
namespace Shogun {

    class ShogunRoundRobinArbitrator : public ShogunArbitrator {

    public:
        ShogunRoundRobinArbitrator();
        ~ShogunRoundRobinArbitrator();

        void moveEvents(const int num_events,
                        const int port_count,
                        ShogunQueue<ShogunEvent*>** inputQueues,
                        int32_t output_slots,
                        ShogunEvent*** outputEvents,
                        uint64_t cycle ) override;

    private:
        int lastStart;

        int nextPort(const int port_count, const int i) const
        {
            return (i + 1) % port_count;
        }

        int convertToPort(const int port_count, const int port) const
        {
            return port % port_count;
        }
    };

}
}

#endif
