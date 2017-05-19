// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_INIT
#define _H_EMBER_INIT

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberInitGenerator : public EmberMessagePassingGenerator {

public:

    EmberInitGenerator(SST::Component* owner, Params& params) :
            EmberMessagePassingGenerator(owner, params, "Init" ),
			m_rank(-1),
			m_size(0)
    { }

    bool generate( std::queue<EmberEvent*>& evQ )
    {
		verbose(CALL_INFO, 1, 0, "\n");

		if ( 0 == m_size ) {
        	enQ_init( evQ );
        	enQ_rank( evQ, GroupWorld, &m_rank );
        	enQ_size( evQ, GroupWorld, &m_size );
			return false;
		} else {
			setRank(m_rank);
			setSize(m_size);
        	return true;
		}
    }

private:
	uint32_t m_rank;
	int m_size;
};

}
}

#endif
