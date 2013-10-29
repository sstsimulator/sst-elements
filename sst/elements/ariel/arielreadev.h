
#ifndef _H_SST_ARIEL_READ_EVENT
#define _H_SST_ARIEL_READ_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielReadEvent : public ArielEvent {

	public:
		ArielReadEvent(uint64_t rAddr, uint32_t length);
		~ArielReadEvent();
		ArielEventType getEventType();
		uint64_t getAddress();
		uint32_t getLength();

	private:
		uint64_t readAddress;
		uint32_t readLength;

};

}
}

#endif
