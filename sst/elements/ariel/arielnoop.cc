
#include <sst_config.h>
#include "arielnoop.h"

using namespace SST::ArielComponent;

ArielNoOpEvent::ArielNoOpEvent() {

}

ArielNoOpEvent::~ArielNoOpEvent() {

}

ArielEventType ArielNoOpEvent::getEventType() {
	return NOOP;
}
