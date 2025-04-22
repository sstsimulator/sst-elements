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


#ifndef _H_EMBER_ALLGATHERV_MOTIF
#define _H_EMBER_ALLGATHERV_MOTIF

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberAllgathervGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberAllgathervGenerator,
        "ember",
        "AllgathervMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a Allgatherv operation with type set to float64 and operation SUM",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.iterations",  "Sets the number of alltoall operations to perform",    "1"},
        {   "arg.count",       "Sets the number of data items",        "1"},
        {   "arg.verify",      "Verify data transfer",        "true"},
    )

	EmberAllgathervGenerator(SST::ComponentId_t, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    uint64_t m_startTime;
    uint64_t m_stopTime;
	uint32_t m_iterations;
    uint32_t m_loopIndex;
    int      m_count;

    void*    m_sendBuf;
    void*    m_recvBuf;
    int      m_sendCnt;
    std::vector<int>    m_recvCnts;
    std::vector<int>    m_recvDsp;

    bool m_verify;
};

}
}

#endif
