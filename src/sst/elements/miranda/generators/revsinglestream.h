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


#ifndef _H_SST_MIRANDA_REV_SINGLE_STREAM_GEN
#define _H_SST_MIRANDA_REV_SINGLE_STREAM_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class ReverseSingleStreamGenerator : public RequestGenerator {

public:
	ReverseSingleStreamGenerator( Component* owner, Params& params );
	~ReverseSingleStreamGenerator();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

private:
	uint64_t startIndex;
	uint64_t stopIndex;
	uint64_t datawidth;
	uint64_t nextIndex;
	uint64_t stride;

	Output*  out;

};

}
}

#endif
