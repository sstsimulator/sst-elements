
#include <sst_config.h>
#include "zfinalizeevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacEventType ZodiacFinalizeEvent::getEventType() {
	return Z_FINALIZE;
}

ZodiacFinalizeEvent::ZodiacFinalizeEvent() {

}
