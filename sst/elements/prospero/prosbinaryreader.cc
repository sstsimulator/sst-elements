
#include "sst_config.h"
#include "prosbinaryreader.h"

using namespace SST::Prospero;

ProsperoBinaryTraceReader::ProsperoBinaryTraceReader( Component* owner, Params& params ) :
	ProsperoTraceReader(owner, params) {

	std::string traceFile = params.find_string("file", "");
	traceInput = fopen(traceFile.c_str(), "rb");

	recordLength = sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t) + sizeof(uint32_t);
	buffer = (char*) malloc(sizeof(char) * recordLength);
};

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
