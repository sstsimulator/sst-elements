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


#ifndef _H_SST_ARIEL_COMPRESSED_BINARY_TRACE_GEN
#define _H_SST_ARIEL_COMPRESSED_BINARY_TRACE_GEN

#include <climits>

#include <sst/core/params.h>
#include "zlib.h"
#include "arieltracegen.h"

namespace SST {
namespace ArielComponent {

class ArielCompressedBinaryTraceGenerator : public ArielTraceGenerator {

public:
	ArielCompressedBinaryTraceGenerator(Component* owner, Params& params) :
		ArielTraceGenerator() {

		tracePrefix = params.find<std::string>("trace_prefix", "ariel-core");
		coreID = 0;

		buffer = (char*) malloc(sizeof(uint64_t) + sizeof(uint64_t) + sizeof(char) + sizeof(uint32_t));
	}

	~ArielCompressedBinaryTraceGenerator() {
		gzclose(traceFile);
		free(buffer);
	}

	void publishEntry(const uint64_t picoS,
                const uint64_t physAddr,
		const uint32_t reqLength,
                const ArielTraceEntryOperation op) {

		const char op_type = (READ == op) ? 'R' : 'W';

		copy(&buffer[0], &picoS, sizeof(uint64_t));
		copy(&buffer[sizeof(uint64_t)], &op_type, sizeof(char));
		copy(&buffer[sizeof(uint64_t) + sizeof(char)], &physAddr, sizeof(uint64_t));
		copy(&buffer[sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t)], &reqLength, sizeof(uint32_t));

		gzwrite(traceFile, buffer, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t) + sizeof(uint32_t));
	}

	void setCoreID(const uint32_t core) {
		coreID = core;

		char* tracePath = (char*) malloc(sizeof(char) * PATH_MAX);
		sprintf(tracePath, "%s-%" PRIu32 ".trace.gz", tracePrefix.c_str(), core);

		traceFile = gzopen(tracePath, "wb");

		free(tracePath);
	}

private:
	void copy(char* dest, const void* src, const size_t length) {
		const char* src_c = (char*) src;

		for(size_t i = 0; i < length; ++i) {
			dest[i] = src_c[i];
		}
	}

	gzFile traceFile;
	std::string tracePrefix;
	uint32_t coreID;
	char* buffer;

};

}
}

#endif
