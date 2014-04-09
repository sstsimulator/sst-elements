
#include <sst_config.h>
#include "zinitevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacInitEvent::ZodiacInitEvent()
{
}

ZodiacEventType ZodiacInitEvent::getEventType() {
	return Z_INIT;
}

