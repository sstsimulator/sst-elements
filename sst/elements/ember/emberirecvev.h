
#ifndef _H_EMBER_IRECV_EVENT
#define _H_EMBER_IRECV_EVENT

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberIRecvEvent : public EmberEvent {

public:
	EmberIRecvEvent(uint32_t rank, uint32_t recvSizeBytes, int tag, Communicator comm, MessageRequest* req);
	~EmberIRecvEvent();
	EmberEventType getEventType();
	uint32_t getRecvFromRank();
	uint32_t getMessageSize();
	MessageRequest* getMessageRequestHandle();
	int getTag();
	std::string getPrintableString();
	Communicator getCommunicator();

protected:
	MessageRequest* request;
	uint32_t recvSizeBytes;
	uint32_t rank;
	int recvTag;
	Communicator comm;

};

}
}

#endif
