
#ifndef _H_SHOGUN_ARB_H
#define _H_SHOGUN_ARB_H

#include "shogun_event.h"

namespace SST {
namespace Shogun {

class ShogunArbitrator {

public:
	ShogunArbitrator() {}
	virtual ~ShogunArbitrator() {}

	virtual void moveEvents( const int master_port_count, const int slave_port_count,
		ShogunEvent** inputEvents, ShogunEvent** outputEvents, uint64_t cycle ) = 0;
};

}
}

#endif
