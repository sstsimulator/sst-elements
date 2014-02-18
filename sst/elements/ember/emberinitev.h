
#ifndef _H_EMBER_INIT_EVENT
#define _H_EMBER_INIT_EVENT

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberInitEvent : EmberEvent {

public:
	EmberInitEvent();
	~EmberInitEvent();
	EmberEventType getEventType();

};

}
}

#endif
