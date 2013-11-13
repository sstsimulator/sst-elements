
#ifndef _H_SST_ARIEL_ALLOC_EVENT
#define _H_SST_ARIEL_ALLOC_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielAllocateEvent : public ArielEvent {

	public:
		ArielAllocateEvent(uint64_t vAddr, uint64_t len, uint32_t lev);
		~ArielAllocateEvent();
		ArielEventType getEventType();
		uint64_t getVirtualAddress();
		uint64_t getAllocationLength();
		uint32_t getAllocationLevel();

	protected:
		uint64_t virtualAddress;
		uint64_t allocateLength;
		uint32_t level;

};

}
}

#endif
