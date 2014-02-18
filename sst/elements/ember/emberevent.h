
#ifndef _H_EMBER_EVENT
#define _H_EMBER_EVENT

namespace SST {
namespace Ember {

enum EmberEventType {
	INIT,
	FINALIZE,
	SEND,
	RECV,
	IRECV
};

class EmberEvent {

public:
	EmberEvent();
	~EmberEvent();
	virtual EmberEventType getEventType() = 0;

};

}
}

#endif
