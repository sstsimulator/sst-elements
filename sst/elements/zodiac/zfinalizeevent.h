
#ifndef _H_ZODIAC_FINALIZE_EVENT
#define _H_ZODIAC_FINALIZE_EVENT

#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacFinalizeEvent : public ZodiacEvent {

	public:
		ZodiacFinalizeEvent();
		ZodiacEventType getEventType();

	private:

};

}
}

#endif
