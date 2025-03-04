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


#ifndef _H_EMBER_STOP_MOTIF
#define _H_EMBER_STOP_MOTIF

#include <sst/core/component.h>
#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberStopGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberStopGenerator,
        "ember",
        "StopMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "NetworkSim: Performs a Barrier Motif and gives a fatal",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.iterations",       "Sets the number of barrier operations to perform",     "1024"},
        {   "arg.compute",      "Sets the time spent computing",        "1"},
    )

	EmberStopGenerator(SST::ComponentId_t, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ );

private:
    uint32_t m_loopIndex;
    uint32_t m_iterations;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    uint64_t m_compute;
    int jobId;

};

}
}

#endif
