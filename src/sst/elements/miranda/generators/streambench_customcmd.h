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


#ifndef _H_SST_MIRANDA_STREAM_BENCH_GEN_CUSTOMCMD
#define _H_SST_MIRANDA_STREAM_BENCH_GEN_CUSTOMCMD

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class STREAMBenchGenerator_CustomCmd : public RequestGenerator {

public:
	STREAMBenchGenerator_CustomCmd( Component* owner, Params& params );
	~STREAMBenchGenerator_CustomCmd();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

private:
	uint64_t reqLength;

	uint64_t start_a;
	uint64_t start_b;
	uint64_t start_c;

	uint64_t n;
	uint64_t n_per_call;
	uint64_t i;

        uint32_t custom_write_opcode;
        uint32_t custom_read_opcode;

	Output*  out;

};

}
}

#endif
