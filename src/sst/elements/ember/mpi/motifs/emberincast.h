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


#ifndef _H_EMBER_INCAST
#define _H_EMBER_INCAST

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberIncastGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberIncastGenerator,
        "ember",
        "IncastMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a message incast Motif",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.messageSize",      "Sets the message size of the ping pong operation", "1024"},
        {   "arg.iterations",       "Sets the number of ping pong operations to perform",   "1"},
        {   "arg.incasttarget",     "Sets the incast target for communications",    "0"},
    )

	EmberIncastGenerator(SST::ComponentId_t, Params& params);
    	bool generate( std::queue<EmberEvent*>& evQ);

private:
    	MessageRequest*   m_req;
    	char*    	  m_sendBuf;
    	char*    	  m_recvBuf;

	uint32_t 	  m_messageSize;
	uint32_t 	  m_iterations;
    	uint32_t 	  m_currentItr;

    	int      	  m_incastTarget;
};

}
}

#endif
