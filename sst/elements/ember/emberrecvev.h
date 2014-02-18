
#ifndef _H_EMBER_RECV_EVENT
#define _H_EMBER_RECV_EVENT

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberRecvEvent : EmberEvent {

public:
	EmberRecvEvent(uint32_t rank, uint32_t recvSizeBytes, int tag);
	~EmberRecvEvent();
	EmberEventType getEventType();
	uint32_t getRecvFromRank();
	uint32_t getMessageSize();
	int getTag();

protected:
	uint32_t recvSizeBytes;
	uint32_t rank;
	int recvTag;

};

}
}

#endif
