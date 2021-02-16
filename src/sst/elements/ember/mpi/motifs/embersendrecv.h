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


#ifndef _H_EMBER_SENDRECV
#define _H_EMBER_SENDRECV

#include "mpi/embermpigen.h"
#include "rng/xorshift.h"

namespace SST {
namespace Ember {


#define DATA_TYPE INT
class EmberSendrecvGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberSendrecvGenerator,
        "ember",
        "SendrecvMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a Sendrecv Motif",
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
        { "time-Sendrecv", "Time spent in Sendrecv event",    "ns", 0},
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
	EmberSendrecvGenerator(SST::ComponentId_t id, Params& params):
        EmberMessagePassingGenerator(id, params, "Null" ), m_phase(Init)
	{
			m_messageSize = 1024;
   	}

    bool generate( std::queue<EmberEvent*>& evQ){
		assert( size() == 2 );
		switch ( m_phase ) {
			case Init:
		    	m_sendBuf = memAlloc(m_messageSize * sizeofDataType(DATA_TYPE) );
    			m_recvBuf = memAlloc(m_messageSize * sizeofDataType(DATA_TYPE) );
				enQ_sendrecv( evQ,
							m_sendBuf, m_messageSize, DATA_TYPE, destRank(), 0xdeadbeef,
							m_recvBuf, m_messageSize, DATA_TYPE, srcRank(),  0xdeadbeef,
							GroupWorld, &m_resp );
				m_phase = Fini;
				return false;

			case Fini:
				printf("rand %d done\n",rank());
				return true;

		}
		assert(0);
	}
	int destRank() {
		return (rank() + 1) % 2;
	}
	int srcRank() {
		return (rank() + 1) % 2;
	}

private:

	int      m_messageSize;
    void*    m_sendBuf;
    void*    m_recvBuf;
	enum { Init, Fini } m_phase;
    MessageResponse m_resp;
};

}
}

#endif
