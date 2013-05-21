
#ifndef _H_PROFILE_READER
#define _H_PROFILE_READER

#include "isisevent.h"

namespace SST {
namespace IsisComponent {

class IsisEventGenerator : public SST::Component {
	public:
		virtual IsisEvent generateNextEvent() = 0;

	}
}
}

#endif
