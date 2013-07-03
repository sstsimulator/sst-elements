
#ifndef _H_ZODIAC_COMPUTE_EVENT
#define _H_ZODIAC_COMPUTE_EVENT

#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacComputeEvent : public ZodiacEvent {

	public:
		ZodiacComputeEvent(double timeSeconds);
		double getComputeDuration();
		double getComputeDurationNano();
		ZodiacEventType getEventType();

	private:
		double computeTime;

};

}
}

#endif
