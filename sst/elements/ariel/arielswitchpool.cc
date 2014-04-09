
#include <sst_config.h>
#include "arielswitchpool.h"

using namespace SST::ArielComponent;

ArielSwitchPoolEvent::ArielSwitchPoolEvent(uint32_t newPool) {
	pool = newPool;
}

ArielSwitchPoolEvent::~ArielSwitchPoolEvent() {

}

ArielEventType ArielSwitchPoolEvent::getEventType() {
	return SWITCH_POOL;
}

uint32_t ArielSwitchPoolEvent::getPool() {
	return pool;
}
