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


#ifndef _H_EMBER_HALO_2DNBR
#define _H_EMBER_HALO_2DNBR

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberHalo2DNBRGenerator : public EmberMessagePassingGenerator {

public:
	EmberHalo2DNBRGenerator(SST::Component* owner, Params& params);
	void configure();
	bool generate( std::queue<EmberEvent*>& evQ);
    void completed(const SST::Output* output, uint64_t );

private:
	uint32_t m_loopIndex;
	uint32_t sizeX;
	uint32_t sizeY;
	uint32_t nsCompute;
//	uint32_t nsCopyTime;
	uint32_t messageSizeX;
	uint32_t messageSizeY;
	uint32_t iterations;
	uint32_t messageCount;

	bool sendWest;
	bool sendEast;
	bool sendNorth;
	bool sendSouth;

	int32_t  procEast;
	int32_t  procWest;
	int32_t  procNorth;
	int32_t  procSouth;
};

}
}

#endif
