
#ifndef _H_SST_ARIEL_FREE_EVENT
#define _H_SST_ARIEL_FREE_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielFreeEvent : public ArielEvent {

	public:
		ArielFreeEvent(uint64_t vAddr);
		~ArielFreeEvent();
		ArielEventType getEventType();
		uint64_t getVirtualAddress();

	protected:
		uint64_t virtualAddress;

};

}
}

#endif
