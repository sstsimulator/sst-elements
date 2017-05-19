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


#ifndef _H_EMBER_TRAFFIC_GEN
#define _H_EMBER_TRAFFIC_GEN

#include <sst/core/rng/gaussian.h>

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberTrafficGenGenerator : public EmberMessagePassingGenerator {

public:
	EmberTrafficGenGenerator(SST::Component* owner, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);
    bool primary( ) {
        return false;
    }
    void configure();

private:
//    MessageResponse m_resp;
    double  m_mean;
    double  m_stddev;
    double  m_startDelay;
    void*    m_sendBuf;
    void*    m_recvBuf;
    MessageRequest m_req;

	uint32_t m_messageSize;
    SSTGaussianDistribution* m_random;
};

}
}

#endif
