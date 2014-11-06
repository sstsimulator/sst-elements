
#ifndef _H_SST_PROSPERO_TEXT_READER
#define _H_SST_PROSPERO_TEXT_READER

#include "prosreader.h"

using namespace SST::Prospero;

namespace SST {
namespace Prospero {

class ProsperoTextTraceReader : public ProsperoTraceReader {

public:
        ProsperoTextTraceReader( Component* owner, Params& params );
        ~ProsperoTextTraceReader();
        ProsperoTraceEntry* readNextEntry();

private:
	FILE* traceInput;

};

}
}

#endif
