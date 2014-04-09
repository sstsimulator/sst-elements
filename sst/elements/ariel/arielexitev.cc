
#include <sst_config.h>
#include "arielexitev.h"

using namespace SST::ArielComponent;

ArielExitEvent::ArielExitEvent() {

}

ArielExitEvent::~ArielExitEvent() {

}

ArielEventType ArielExitEvent::getEventType() {
	return CORE_EXIT;
}
