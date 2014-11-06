
#include "prostextreader.h"

ProsperoBinaryTraceReader::ProsperoBinaryTraceReader( Component* owner, Params& params ) {
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

void ProsperoBinaryTraceReader::copy(char* target, const char* source, const uint32_t len) {
	for(uint32_t i = 0; i < len; ++i) {
		target[i] = source[i];
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

	if(1 == fread(buffer, recordLength, 1, traceInput)) {
		// We DID read an entry
		copy(&reqCycles, buffer, 0, sizeof(uint64_t));
		copy(&reqType, buffer, sizeof(uint64_t), sizeof(char));
		copy(&reqAddress, buffer, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t));
		copy(&reqLength, buffer, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t));

		return new ProsperoTraceEntry(reqCycles, reqAddress,
			reqType == 'R' ? READ : WRITE,
			reqLength);
	} else {
		// Did not get a full read?
		return NULL;
	}
}
