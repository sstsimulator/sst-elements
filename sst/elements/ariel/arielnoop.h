
#ifndef _H_SST_ARIEL_NOOP_EVENT
#define _H_SST_ARIEL_NOOP_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielNoOpEvent : public ArielEvent {

	public:
		ArielNoOpEvent();
		~ArielNoOpEvent();
		ArielEventType getEventType();

};

}
}

#endif
