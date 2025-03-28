// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberInitGenerator,
        "ember",
        "InitMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a communication Initialization Motif",
        SST::Ember::EmberInitGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    EmberInitGenerator(SST::ComponentId_t id, Params& params) :
            EmberMessagePassingGenerator(id, params, "Init" ),
			m_rank(-1),
			m_size(0)
    { }

    bool generate( std::queue<EmberEvent*>& evQ )
    {
		if ( 0 == m_size ) {
			verbose(CALL_INFO, 1, MOTIF_MASK, "\n");
        	enQ_init( evQ );
        	enQ_rank( evQ, GroupWorld, &m_rank );
        	enQ_size( evQ, GroupWorld, &m_size );
			return false;
		} else {
			verbose(CALL_INFO, 1, MOTIF_MASK, "size=%d rank=%d\n",m_size,m_rank);
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
