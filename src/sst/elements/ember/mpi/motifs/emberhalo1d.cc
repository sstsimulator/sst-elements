// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>

#include "emberhalo1d.h"

using namespace SST::Ember;

EmberHalo1DGenerator::EmberHalo1DGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "Halo1D"),
	m_loopIndex(0)
{
	iterations = (uint32_t) params.find("arg.iterations", 10);
	nsCompute = (uint32_t) params.find("arg.computenano", 1000);
	messageSize = (uint32_t) params.find("arg.messagesize", 128);
}

bool EmberHalo1DGenerator::generate( std::queue<EmberEvent*>& evQ ) {

    if( 0 == m_loopIndex) {
        verbose(CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(),size());
    }

		enQ_compute( evQ, nsCompute );

		if(0 == rank()) {
			enQ_recv( evQ, 1, messageSize, 0, GroupWorld);
			enQ_send( evQ, 1, messageSize, 0, GroupWorld);

		} else if( (size() - 1) == (signed )rank() ) {
			enQ_send( evQ, rank() - 1, messageSize, 0, GroupWorld);
			enQ_recv( evQ, rank() - 1, messageSize, 0, GroupWorld);

		} else {
			enQ_send( evQ, rank() - 1, messageSize, 0, GroupWorld);
			enQ_recv( evQ, rank() + 1, messageSize, 0, GroupWorld);
			enQ_send( evQ, rank() + 1, messageSize, 0, GroupWorld);
			enQ_recv( evQ, rank() - 1, messageSize, 0, GroupWorld);
		}

    if ( ++m_loopIndex == iterations ) {
        return true;
    } else {
        return false;
    }

}
