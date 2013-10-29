
#ifndef _H_SST_ARIEL_WRITE_EVENT
#define _H_SST_ARIEL_WRITE_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielWriteEvent : public ArielEvent {

	public:
		ArielWriteEvent(uint64_t wAddr, uint32_t length);
		~ArielWriteEvent();
		ArielEventType getEventType();
		uint64_t getAddress();
		uint32_t getLength();

	private:
		uint64_t writeAddress;
		uint32_t writeLength;

};

}
}

#endif
