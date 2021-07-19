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
#include "prostextreader.h"

using namespace SST::Prospero;


ProsperoTextTraceReader::ProsperoTextTraceReader( ComponentId_t id, Params& params, Output* out ) :
	ProsperoTraceReader(id, params, out) {

	std::string traceFile = params.find<std::string>("file", "");
	traceInput = fopen(traceFile.c_str(), "rt");

	if(NULL == traceInput) {
            output->fatal(CALL_INFO, -1, "%s, Fatal: Unable to open file: %s in text reader.\n",
                    getName().c_str(), traceFile.c_str());
	}

}

ProsperoTextTraceReader::~ProsperoTextTraceReader() {
	if(NULL != traceInput) {
		fclose(traceInput);
	}
}

ProsperoTraceEntry* ProsperoTextTraceReader::readNextEntry() {
	uint64_t reqAddress = 0;
	uint64_t reqCycles  = 0;
	char reqType = 'R';
	uint32_t reqLength  = 0;

	if(EOF == fscanf(traceInput, "%" PRIu64 " %c %" PRIu64 " %" PRIu32 "",
		&reqCycles, &reqType, &reqAddress, &reqLength) ) {
		return NULL;
	} else {
		return new ProsperoTraceEntry(reqCycles, reqAddress,
			reqLength,
			(reqType == 'R' || reqType == 'r') ? READ : WRITE);
	}
}
