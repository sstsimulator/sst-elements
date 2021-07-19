// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "prosbinaryreader.h"

using namespace SST::Prospero;


ProsperoBinaryTraceReader::ProsperoBinaryTraceReader( ComponentId_t id, Params& params, Output* out ) :
	ProsperoTraceReader(id, params, out) {

	std::string traceFile = params.find<std::string>("file", "");
	traceInput = fopen(traceFile.c_str(), "rb");

	if(NULL == traceInput) {
            output->fatal(CALL_INFO, -1, "%s, Fatal: Error opening trace file: %s in binary reader.\n",
                    getName().c_str(), traceFile.c_str());
	}

	recordLength = sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t) + sizeof(uint32_t);
	buffer = (char*) malloc(sizeof(char) * recordLength);
}

ProsperoBinaryTraceReader::~ProsperoBinaryTraceReader() {
	if(NULL != traceInput) {
		fclose(traceInput);
	}

	if(NULL != buffer) {
		free(buffer);
	}
}

void ProsperoBinaryTraceReader::copy(char* target, const char* source,
	const size_t bufferOffset, const size_t len) {

	for(size_t i = 0; i < len; ++i) {
		target[i] = source[bufferOffset + i];
	}
}

ProsperoTraceEntry* ProsperoBinaryTraceReader::readNextEntry() {
	uint64_t reqAddress = 0;
	uint64_t reqCycles  = 0;
	char reqType = 'R';
	uint32_t reqLength  = 0;

	if(feof(traceInput)) {
		return NULL;
	}

	if(1 == fread(buffer, (size_t) recordLength, (size_t) 1, traceInput)) {
		// We DID read an entry
		copy((char*) &reqCycles,  buffer, (size_t) 0, sizeof(uint64_t));
		copy((char*) &reqType,    buffer, sizeof(uint64_t), sizeof(char));
		copy((char*) &reqAddress, buffer, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t));
		copy((char*) &reqLength,  buffer, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t));

		return new ProsperoTraceEntry(reqCycles, reqAddress,
			reqLength,
			(reqType == 'R' || reqType == 'r') ? READ : WRITE);
	} else {
		// Did not get a full read?
		return NULL;
	}
}
