
#include "prosreader.h"

class ProsperoTextTraceReader : public ProsperoTraceReader {

public:
        ProsperoTextTraceReader( Component* owner, Params& params ) { }
        ~ProsperoTextTraceReader() { }
        ProsperoTraceEntry* readNextEntry() { return NULL; }

private:
	FILE* traceInput;

};
