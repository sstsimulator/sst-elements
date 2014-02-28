
#ifndef _H_EMBER_EVENT
#define _H_EMBER_EVENT

#include <sst/core/event.h>
#include <stdint.h>
#include <string>

namespace SST {
namespace Ember {

enum EmberEventType {
	INIT,
	FINALIZE,
	SEND,
	RECV,
	IRECV,
	COMPUTE,
	START
};

class EmberEvent : public SST::Event {

public:
	EmberEvent();
	~EmberEvent();
	virtual EmberEventType getEventType() = 0;
	virtual std::string getPrintableString() = 0;

};

}
}

#endif
