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


#ifndef _H_SST_MIRANDA_STENCIL3D_BENCH_GEN
#define _H_SST_MIRANDA_STENCIL3D_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class Stencil3DBenchGenerator : public RequestGenerator {

public:
	Stencil3DBenchGenerator( Component* owner, Params& params );
	~Stencil3DBenchGenerator();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

private:
	void convertIndexToPosition(const uint32_t index,
		uint32_t* posX, uint32_t* posY, uint32_t* posZ);
	uint32_t convertPositionToIndex(const uint32_t posX,
		const uint32_t posY, const uint32_t posZ);

	uint32_t nX;
	uint32_t nY;
	uint32_t nZ;
	uint32_t datawidth;

	uint32_t startZ;
	uint32_t endZ;

	uint32_t currentZ;
	uint32_t currentItr;
	uint32_t maxItr;

	Output*  out;

};

}
}

#endif
