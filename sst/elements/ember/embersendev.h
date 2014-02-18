
#ifndef _H_EMBER_SEND_EVENT
#define _H_EMBER_SEND_EVENT

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberSendEvent : EmberEvent {

public:
	EmberSendEvent(uint32_t rank, uint32_t sendSizeBytes, int tag);
	~EmberSendEvent();
	EmberEventType getEventType();
	uint32_t getSendToRank();
	uint32_t getMessageSize();
	int getTag();

protected:
	uint32_t sendSizeBytes;
	uint32_t rank;
	int sendTag;

};

}
}

#endif
