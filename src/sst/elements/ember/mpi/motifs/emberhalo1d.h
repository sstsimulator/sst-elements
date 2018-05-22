// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_HALO_1D
#define _H_EMBER_HALO_1D

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberHalo1DGenerator : public EmberMessagePassingGenerator {

public:
	EmberHalo1DGenerator(SST::Component* owner, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ );

private:
	uint32_t m_loopIndex;
	uint32_t nsCompute;
	uint32_t messageSize;
	uint32_t iterations;
//	uint32_t wrapAround;

};

}
}

#endif
