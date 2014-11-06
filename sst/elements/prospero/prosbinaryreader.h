
#ifndef _H_SST_PROSPERO_BINARY_READER
#define _H_SST_PROSPERO_BINARY_READER

#include "prosreader.h"

namespace SST {
namespace Prospero {

class ProsperoBinaryTraceReader : public ProsperoTraceReader {

public:
        ProsperoBinaryTraceReader( Component* owner, Params& params );
        ~ProsperoBinaryTraceReader();
        ProsperoTraceEntry* readNextEntry();

private:
	void copy(char* target, const char* source,
		const size_t offset, const size_t len);
	FILE* traceInput;
	char* buffer;
	uint32_t recordLength;

};

}
}

#endif
