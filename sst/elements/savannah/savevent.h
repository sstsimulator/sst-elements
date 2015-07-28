
#ifndef _H_SST_SAVANNAH_EVENT
#define _H_SST_SAVANNAH_EVENT

#include <sst/elements/memHierarchy/memoryController.h>
#include <sst/elements/memHierarchy/membackend/memBackend.h>

using namespace SST::MemHierarchy;

namespace SST {
namespace Savannah {

class SavannahRequestEvent : public SST::Event {
public:
	SavannahRequestEvent(MemController::DRAMReq& req) :
		request(req) {

		recvLink = 0;
	};

	MemController::DRAMReq& getRequest() {
		return request;
	}

	MemController::DRAMReq* getRequestPtr() {
		return &request;
	}

	uint32_t getLink() const {
		return recvLink;
	}

	void setLink(const uint32_t linkID) {
		recvLink = linkID;
	}

private:
	MemController::DRAMReq request;
	uint32_t recvLink;
};

}
}

#endif
