// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ARIEL_TEXT_TRACE_GEN
#define _H_SST_ARIEL_TEXT_TRACE_GEN

#include <sst/core/params.h>
#include "arieltracegen.h"

namespace SST {
namespace ArielComponent {

class ArielTextTraceGenerator : public ArielTraceGenerator {

public:
	ArielTextTraceGenerator(Component* owner, Params& params) :
		ArielTraceGenerator() {

		tracePrefix = params.find<std::string>("trace_prefix", "ariel-core");
		coreID = 0;
	}

	~ArielTextTraceGenerator() {
		fclose(textFile);
	}

	void publishEntry(const uint64_t picoS,
                const uint64_t physAddr,
		const uint32_t reqLength,
                const ArielTraceEntryOperation op) {

		fprintf(textFile, "%" PRIu64 " %s %" PRIu64 " %" PRIu32 "\n",
			picoS,
			(op == READ) ? "R" : "W",
			physAddr,
			reqLength);
	}

	void setCoreID(const uint32_t core) {
		coreID = core;

		char* tracePath = (char*) malloc(sizeof(char) * PATH_MAX);
		sprintf(tracePath, "%s-%" PRIu32 ".trace", tracePrefix.c_str(), core);

		textFile = fopen(tracePath, "wt");

		free(tracePath);
	}

private:
	FILE* textFile;
	std::string tracePrefix;
	uint32_t coreID;

};

}
}

#endif
