
#include "sst_config.h"
#include "prostextreader.h"

using namespace SST::Prospero;

ProsperoTextTraceReader::ProsperoTextTraceReader( Component* owner, Params& params ) :
	ProsperoTraceReader(owner, params) {

	std::string traceFile = params.find_string("file", "");
	traceInput = fopen(traceFile.c_str(), "rt");

	if(NULL == traceInput) {
		fprintf(stderr, "Fatal: Unable to open file: %s in text reader.\n",
			traceFile.c_str());
		exit(-1);
	}

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
		&reqCycles, &reqType, &reqAddress, &reqLength) ) {
		return NULL;
	} else {
		return new ProsperoTraceEntry(reqCycles, reqAddress,
			reqLength,
			(reqType == 'R' || reqType == 'r') ? READ : WRITE);
	}
}
