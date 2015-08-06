
#ifndef _H_SST_SAVANNAH_EVENT
#define _H_SST_SAVANNAH_EVENT

#include "sst/elements/memHierarchy/memoryController.h"
#include "sst/elements/memHierarchy/membackend/memBackend.h"
#include "sst/elements/memHierarchy/DRAMReq.h"

using namespace SST::MemHierarchy;

namespace SST {
namespace Savannah {

class SavannahRequestEvent : public SST::Event {
public:
	SavannahRequestEvent(DRAMReq& req) :
		request(req) {

		recvLink = 0;
	};

	DRAMReq& getRequest() {
		return request;
	}

	DRAMReq* getRequestPtr() {
		return &request;
	}

	uint32_t getLink() const {
		return recvLink;
	}

	void setLink(const uint32_t linkID) {
		recvLink = linkID;
	}

private:
	DRAMReq request;
	uint32_t recvLink;
};

}
}

#endif
