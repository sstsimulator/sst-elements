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


#ifndef _H_EMBER_SENDRECV
#define _H_EMBER_SENDRECV

#include "mpi/embermpigen.h"
#include "rng/xorshift.h"

namespace SST {
namespace Ember {


#define DATA_TYPE INT
class EmberSendrecvGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberSendrecvGenerator,
        "ember",
        "SendrecvMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a Sendrecv Motif",
        SST::Ember::EmberGenerator
    )

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
