
#ifndef _H_EMBER_SEND_EVENT
#define _H_EMBER_SEND_EVENT

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberSendEvent : public EmberEvent {

public:
	EmberSendEvent(uint32_t rank, uint32_t sendSizeBytes, int tag, Communicator comm);
	~EmberSendEvent();
	EmberEventType getEventType();
	uint32_t getSendToRank();
	uint32_t getMessageSize();
	int getTag();
	Communicator getCommunicator();
	std::string getPrintableString();

protected:
	uint32_t sendSizeBytes;
	uint32_t rank;
	int sendTag;
	Communicator comm;

};

}
}

#endif
