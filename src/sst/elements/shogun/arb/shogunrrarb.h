
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

	void moveEvents( const int master_port_count, const int slave_port_count,
		ShogunEvent** inputEvents, ShogunEvent** outputEvents, uint64_t cycle ) {

		const int total_port_count = master_port_count + slave_port_count;

		for( int i = 0; i < total_port_count; ++i ) {
			const int currentIndex = (i + lastStart) % (total_port_count);
			const int dest = inputEvents[currentIndex]->getDestination();

			if( nullptr == outputEvents[dest] ) {
				outputEvents[dest] = inputEvents[currentIndex];
				inputEvents[currentIndex] = nullptr;
			}
		}

		lastStart++;
	}

private:
	int lastStart;

};

}
}

#endif
