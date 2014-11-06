
#include "prostextreader.h"

ProsperoTextTraceReader::ProsperoTextTraceReader( Component* owner, Params& params ) {
	std::string traceFile = params.find_string("file", "");
	traceInput = fopen(traceFile.c_str(), "rt");

};

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
		&reqCycles, &reqType, &reyAddress, &reqLength) ) {
		return NULL;
	} else {
		return new ProsperoTraceEntry(reqCycles, reqAddress,
			reqType == 'R' ? READ : WRITE,
			reqLength);
	}
}
