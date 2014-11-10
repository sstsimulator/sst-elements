
#include "sst_config.h"
#include "prosbingzreader.h"

using namespace SST::Prospero;

ProsperoCompressedBinaryTraceReader::ProsperoCompressedBinaryTraceReader( Component* owner, Params& params ) :
	ProsperoTraceReader(owner, params) {

	std::string traceFile = params.find_string("file", "");
	traceInput = gzopen(traceFile.c_str(), "rb");

	if(Z_NULL == traceInput) {
		fprintf(stderr, "Fatal: attempted to open: %s but zlib returns error condition.\n",
			traceFile.c_str());
		exit(-1);
	}

	recordLength = sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t) + sizeof(uint32_t);
	buffer = (char*) malloc(sizeof(char) * recordLength);
};

ProsperoCompressedBinaryTraceReader::~ProsperoCompressedBinaryTraceReader() {
	if(NULL != traceInput) {
		gzclose(traceInput);
	}

	if(NULL != buffer) {
		free(buffer);
	}
}

void ProsperoCompressedBinaryTraceReader::copy(char* target, const char* source,
	const size_t bufferOffset, const size_t len) {

	for(size_t i = 0; i < len; ++i) {
		target[i] = source[bufferOffset + i];
	}
}

ProsperoTraceEntry* ProsperoCompressedBinaryTraceReader::readNextEntry() {
	output->verbose(CALL_INFO, 4, 0, "Reading next trace entry...\n");

	uint64_t reqAddress = 0;
	uint64_t reqCycles  = 0;
	char reqType = 'R';
	uint32_t reqLength  = 0;

	if(gzeof(traceInput)) {
		output->verbose(CALL_INFO, 2, 0, "End of trace file reached, returning empty request.\n");
		return NULL;
	}

	int bytesRead = gzread(traceInput, buffer, (unsigned int) recordLength);

	if(bytesRead == recordLength) {
		// We DID read an entry
		copy((char*) &reqCycles,  buffer, (size_t) 0, sizeof(uint64_t));
		copy((char*) &reqType,    buffer, sizeof(uint64_t), sizeof(char));
		copy((char*) &reqAddress, buffer, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t));
		copy((char*) &reqLength,  buffer, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t));

		return new ProsperoTraceEntry(reqCycles, reqAddress,
			reqLength,
			(reqType == 'R' || reqType == 'r') ? READ : WRITE);
	} else {
		output->verbose(CALL_INFO, 2, 0, "Did not read a full record from the compressed trace, returning empty request.\n");
		// Did not get a full read?
		return NULL;
	}
}
