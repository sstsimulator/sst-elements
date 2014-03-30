
#ifndef _H_EMBER_BARRIER_EV
#define _H_EMBER_BARRIER_EV

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberBarrierEvent : public EmberEvent {
	public:
		EmberBarrierEvent(Communicator comm);
		~EmberBarrierEvent();
		EmberEventType getEventType();
	        std::string getPrintableString();
		Communicator getCommunicator();

	private:
		Communicator comm;
};

}
}

#endif
