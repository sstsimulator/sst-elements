
#ifndef _H_SST_ARIEL_EXIT_EVENT
#define _H_SST_ARIEL_EXIT_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielExitEvent : public ArielEvent {

	public:
		ArielExitEvent();
		~ArielExitEvent();
		ArielEventType getEventType();

};

}
}

#endif
