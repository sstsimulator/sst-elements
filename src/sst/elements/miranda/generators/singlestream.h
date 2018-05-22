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


#ifndef _H_SST_MIRANDA_SINGLE_STREAM_GEN
#define _H_SST_MIRANDA_SINGLE_STREAM_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class SingleStreamGenerator : public RequestGenerator {

public:
	SingleStreamGenerator( Component* owner, Params& params );
	~SingleStreamGenerator();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

	SST_ELI_REGISTER_SUBCOMPONENT(
		SingleStreamGenerator,
                "miranda",
                "SingleStreamGenerator",
                SST_ELI_ELEMENT_VERSION(1,0,0),
		"Creates a single reverse ordering stream of accesses to/from memory",
                "SST::Miranda::RequestGenerator"
        )

	SST_ELI_DOCUMENT_PARAMS(
		{ "start_at",         "Sets the start *index* for this generator", "2048" },
    		{ "stop_at",          "Sets the stop *index* for this generator, stop < start", "0" },
    		{ "verbose",          "Sets the verbosity of the output", "0" },
    		{ "datawidth",        "Sets the width of the memory operation", "8" },
    		{ "stride",           "Sets the stride, since this is a reverse stream this is subtracted per iteration, def=1", "1" }
        )
private:
	uint64_t reqLength;
	uint64_t maxAddr;
	uint64_t issueCount;
	uint64_t nextAddr;
	uint64_t startAddr;
	Output*  out;
	ReqOperation memOp;

};

}
}

#endif
