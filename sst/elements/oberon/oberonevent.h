
#ifndef _H_OBERON_EVENT
#define _H_OBERON_EVENT

namespace SST {
namespace Oberon {

enum OberonEventType {
	HALT
}

class OberonEvent {

	public:
		OberonEvent();
		virtual OberonEventType getEventType() = 0;

}

}
}

#endif