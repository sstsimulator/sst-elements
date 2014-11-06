
#include "prosreader.h"

class ProsperoCompressedBinaryTraceReader : public ProsperoTraceReader {

public:
        ProsperoCompressedBinaryTraceReader( Component* owner, Params& params ) { }
        ~ProsperoCompressedBinaryTraceReader() { }
        ProsperoTraceEntry* readNextEntry() { return NULL; }

private:
	void copy(char* target, const char* source, const uint32_t len);
	FILE* traceInput;
	char* buffer;
	uint32_t recordLength;

};
