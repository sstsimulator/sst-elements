
#ifndef _H_SST_PROSPERO_GZ_BINARY_READER
#define _H_SST_PROSPERO_GZ_BINARY_READER

#include "prosreader.h"
#include "zlib.h"

namespace SST {
namespace Prospero {

class ProsperoCompressedBinaryTraceReader : public ProsperoTraceReader {

public:
        ProsperoCompressedBinaryTraceReader( Component* owner, Params& params );
        ~ProsperoCompressedBinaryTraceReader();
        ProsperoTraceEntry* readNextEntry();

private:
	void copy(char* target, const char* source, const size_t buffOffset, const size_t len);
	gzFile traceInput;
	char* buffer;
	uint32_t recordLength;

};

}
}

#endif
