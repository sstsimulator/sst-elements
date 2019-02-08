
#ifndef _H_SHOGUN_RANDOM_ARB_H
#define _H_SHOGUN_RANDOM_ARB_H

#include "shogun_event.h"
#include "shogunarb.h"

namespace SST {
namespace Shogun {

class ShogunRoundRobinArbitrator : public ShogunArbitrator {

public:
	ShogunRoundRobinArbitrator() {
		lastStart = 0;
	}

	~ShogunRoundRobinArbitrator() {
	}

	void moveEvents( const int port_count,
                ShogunQueue<ShogunEvent*>** inputQueues, ShogunEvent** outputEvents, uint64_t cycle ) override {

        }

private:
	int lastStart;

};

}
}

#endif
