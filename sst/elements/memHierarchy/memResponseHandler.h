
#ifndef _H_SST_MEM_HIERARCHY_MEM_RESP_HANDLER
#define _H_SST_MEM_HIERARCHY_MEM_RESP_HANDLER

#include "sst/elements/memHierarchy/DRAMReq.h"

namespace SST {
namespace MemHierarchy {

class MemResponseHandler {
	virtual void handleMemResponse(DRAMReq *req) = 0;
};

}
}

#endif
