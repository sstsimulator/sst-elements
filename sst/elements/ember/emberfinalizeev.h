
#ifndef _H_EMBER_FINALIZE_EVENT
#define _H_EMBER_FINALIZE_EVENT

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberFinalizeEvent : public EmberEvent {

public:
	EmberFinalizeEvent();
	~EmberFinalizeEvent();
	EmberEventType getEventType();
	std::string getPrintableString();

};

}
}

#endif
