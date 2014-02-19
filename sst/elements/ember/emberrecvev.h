
#ifndef _H_EMBER_RECV_EVENT
#define _H_EMBER_RECV_EVENT

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberRecvEvent : public EmberEvent {

public:
	EmberRecvEvent(uint32_t rank, uint32_t recvSizeBytes, int tag, Communicator comm);
	~EmberRecvEvent();
	EmberEventType getEventType();
	uint32_t getRecvFromRank();
	uint32_t getMessageSize();
	int getTag();
	std::string getPrintableString();
	Communicator getCommunicator();

protected:
	uint32_t recvSizeBytes;
	uint32_t rank;
	int recvTag;
	Communicator comm;

};

}
}

#endif
