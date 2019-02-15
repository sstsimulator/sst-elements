
#ifndef _H_SHOGUN_ARB_H
#define _H_SHOGUN_ARB_H

#include "shogun_q.h"
#include "shogun_event.h"
#include "shogun_stat_bundle.h"

using namespace SST::Shogun;

namespace SST {
namespace Shogun {

class ShogunArbitrator {

public:
	ShogunArbitrator() {}
	virtual ~ShogunArbitrator() {}

	virtual void moveEvents( const int port_count,
		ShogunQueue<ShogunEvent*>** inputQueues, ShogunEvent** outputEvents, uint64_t cycle ) = 0;

	void setOutput(SST::Output* out) {
		output = out;
	}

	void setStatisticsBundle( ShogunStatisticsBundle* b ) {
		bundle = b;
	}

protected:
	SST::Output* output;
	ShogunStatisticsBundle* bundle;

};

}
}

#endif
