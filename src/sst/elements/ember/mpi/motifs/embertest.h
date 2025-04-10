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


#ifndef _H_EMBER_TEST
#define _H_EMBER_TEST

#include "mpi/embermpigen.h"
#include "rng/xorshift.h"

namespace SST {
namespace Ember {

class EmberTestGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberTestGenerator,
        "ember",
        "TestMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a Test Motif",
        SST::Ember::EmberGenerator
    )

	EmberTestGenerator(SST::ComponentId_t id, Params& params):
        EmberMessagePassingGenerator(id, params, "Null" ), m_phase(Init)
	{
		m_rng = new SST::RNG::XORShiftRNG();
	}
    bool generate( std::queue<EmberEvent*>& evQ){
		assert( size() == 2 );
		switch ( m_phase ) {
			case Init:
				m_rng->seed( rank() + getSeed() );
				if ( rank() == 0 ) {
				 	enQ_irecv( evQ, NULL, 0, CHAR, 1, 0xdeadbeef, GroupWorld, &m_req );
					enQ_test( evQ, &m_req, &m_flag, &m_resp );
					m_phase = Check;
					return false;
				} else {
					enQ_compute( evQ, m_rng->generateNextUInt32() % 1000000 );
					enQ_send( evQ, NULL, 0, CHAR, 0, 0xdeadbeef, GroupWorld );
					return true;
				}

			case Check:

				if ( m_flag ) {
					printf("src=%d\n", m_resp.src );
					return true;
				} else {
					enQ_compute( evQ, 10000 );
					printf("test again\n" );
					enQ_test( evQ, &m_req, &m_flag, &m_resp );
					return false;
				}
		}
		assert(0);
	}

private:

	unsigned int getSeed() {
        struct timeval start;
        gettimeofday( &start, NULL );
        return start.tv_usec;
    }

	SST::RNG::XORShiftRNG* m_rng;
	int m_flag;
	enum { Init, Check } m_phase;
	MessageRequest  m_req;
    MessageResponse m_resp;
};

}
}

#endif
