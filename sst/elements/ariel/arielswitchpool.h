
#ifndef _H_SST_ARIEL_SWITCH_POOL_EVENT
#define _H_SST_ARIEL_SWITCH_POOL_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielSwitchPoolEvent : public ArielEvent {

	public:
		ArielSwitchPoolEvent(uint32_t pool);
		~ArielSwitchPoolEvent();
		ArielEventType getEventType();
		uint32_t getPool();

	private:
		uint32_t pool;

};

}
}

#endif
