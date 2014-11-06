
#include "prosreader.h"

class ProsperoBinaryTraceReader : public ProsperoTraceReader {

public:
        ProsperoBinaryTraceReader( Component* owner, Params& params ) { }
        ~ProsperoBinaryTraceReader() { }
        ProsperoTraceEntry* readNextEntry() { return NULL; }

private:
	void copy(char* target, const char* source, const uint32_t len);
	FILE* traceInput;
	char* buffer;
	uint32_t recordLength;

};
