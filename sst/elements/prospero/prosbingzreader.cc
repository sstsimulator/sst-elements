
#include "prostextreader.h"

ProsperoCompressedBinaryTraceReader::ProsperoCompressedBinaryTraceReader( Component* owner, Params& params ) {
	std::string traceFile = params.find_string("file", "");
	traceInput = gzopen(traceFile.c_str(), "rb");

	recordLength = sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t) + sizeof(uint32_t);
	buffer = (char*) malloc(sizeof(char) * recordLength);
};

ProsperoCompressedBinaryTraceReader::~ProsperoCompressedBinaryTraceReader() {
	if(NULL != traceInput) {
		fclose(traceInput);
	}

	if(NULL != buffer) {
		free(buffer);
	}
}

void ProsperoCompressedBinaryTraceReader::copy(char* target, const char* source, const uint32_t len) {
	for(uint32_t i = 0; i < len; ++i) {
		target[i] = source[i];
	}
}

ProsperoTraceEntry* ProsperoCompressedBinaryTraceReader::readNextEntry() {
	uint64_t reqAddress = 0;
	uint64_t reqCycles  = 0;
	char reqType = 'R';
	uint32_t reqLength  = 0;

	if(gzeof(traceInput)) {
		return NULL;
	}

	if(gzread(traceInput, buffer, recordLength) > 0) {
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
