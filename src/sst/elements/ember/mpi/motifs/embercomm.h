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


#ifndef _H_EMBER_COMM
#define _H_EMBER_COMM

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberCommGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberCommGenerator,
        "ember",
        "CommMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a comm_split test.",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.messagesize",      "Sets the size of the message in bytes",        "0"},
        {   "arg.iterations",       "Sets the number of ping pong operations to perform",   "1"},
    )

	EmberCommGenerator(SST::ComponentId_t, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    MessageResponse m_resp;
    Communicator    m_newComm[2];

    uint32_t m_iterations;
    uint32_t m_messageSize;
    int      m_new_size;
    uint32_t m_new_rank;
    void*    m_sendBuf;
    void*    m_recvBuf;
    int      m_workPhase;
    uint32_t m_loopIndex;
};

}
}

#endif
