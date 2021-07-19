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


#ifndef _H_EMBER_TESTANY
#define _H_EMBER_TESTANY

#include "mpi/embermpigen.h"
#include "rng/xorshift.h"

namespace SST {
namespace Ember {

class EmberTestanyGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberTestanyGenerator,
        "ember",
        "TestanyMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a Testany Motif",
        SST::Ember::EmberGenerator
    )
    SST_ELI_DOCUMENT_STATISTICS(
        { "time-Init", "Time spent in Init event",          "ns",  0},
        { "time-Finalize", "Time spent in Finalize event",  "ns", 0},
        { "time-Rank", "Time spent in Rank event",          "ns", 0},
        { "time-Size", "Time spent in Size event",          "ns", 0},
        { "time-Send", "Time spent in Recv event",          "ns", 0},
        { "time-Recv", "Time spent in Recv event",          "ns", 0},
        { "time-Irecv", "Time spent in Irecv event",        "ns", 0},
        { "time-Isend", "Time spent in Isend event",        "ns", 0},
        { "time-Wait", "Time spent in Wait event",          "ns", 0},
        { "time-Waitall", "Time spent in Waitall event",    "ns", 0},
        { "time-Waitany", "Time spent in Waitany event",    "ns", 0},
        { "time-Test", "Time spent in Test event",    "ns", 0},
        { "time-Testany", "Time spent in Testany event",    "ns", 0},
        { "time-Compute", "Time spent in Compute event",    "ns", 0},
        { "time-Barrier", "Time spent in Barrier event",    "ns", 0},
        { "time-Alltoallv", "Time spent in Alltoallv event", "ns", 0},
        { "time-Alltoall", "Time spent in Alltoall event",  "ns", 0},
        { "time-Allreduce", "Time spent in Allreduce event", "ns", 0},
        { "time-Reduce", "Time spent in Reduce event",      "ns", 0},
        { "time-Bcast", "Time spent in Bcast event",        "ns", 0},
        { "time-Gettime", "Time spent in Gettime event",    "ns", 0},
        { "time-Commsplit", "Time spent in Commsplit event", "ns", 0},
        { "time-Commcreate", "Time spent in Commcreate event", "ns", 0},
    )

public:
	EmberTestanyGenerator(SST::ComponentId_t id, Params& params):
        EmberMessagePassingGenerator(id, params, "Null" ), m_phase(Init)
	{
		m_rng = new SST::RNG::XORShiftRNG();
	}
    bool generate( std::queue<EmberEvent*>& evQ){
		switch ( m_phase ) {
			case Init:
				m_rng->seed( rank() + getSeed() );
				if ( rank() == 0 ) {
					m_req.resize( size() - 1 );
					for ( int i = 0; i < m_req.size(); i++ ) {
				 		enQ_irecv( evQ, NULL, 0, CHAR, i+1, 0xdeadbeef, GroupWorld, &m_req[i] );
					}
					m_flag = false;
					m_phase = Check;
					return false;
				} else {
					enQ_compute( evQ, m_rng->generateNextUInt32() % 1000000 );
					enQ_send( evQ, NULL, 0, CHAR, 0, 0xdeadbeef, GroupWorld );
					return true;
				}

			case Check:

				if ( m_flag ) {
					printf("index=%d src=%d\n", m_indx, m_resp.src );
					return true;
				} else {
					printf("testany\n" );
					enQ_compute( evQ, 1000 );
					enQ_testany( evQ, m_req.size(), &m_req[0], &m_indx, &m_flag, &m_resp );
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
	int m_indx;
	enum { Init, Check } m_phase;
	std::vector<MessageRequest>  m_req;
    MessageResponse m_resp;
};

}
}

#endif
