
#ifndef _H_EMBER_WAIT_EV
#define _H_EMBER_WAIT_EV

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberWaitEvent : public EmberEvent {

	public:
		EmberWaitEvent(MessageRequest* req);
		~EmberWaitEvent();
		MessageRequest* getMessageRequestHandle();
		EmberEventType getEventType();
		std::string getPrintableString();

	private:
		MessageRequest* request;

};

}
}

#endif
