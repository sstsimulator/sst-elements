
#ifndef _H_EMBER_START_EVENT
#define _H_EMBER_START_EVENT

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberStartEvent : public EmberEvent {

public:
	EmberStartEvent();
	~EmberStartEvent();
	EmberEventType getEventType();
	std::string getPrintableString();

};

}
}

#endif
