
#ifndef _H_ZODIAC_EVENT_BASE
#define _H_ZODIAC_EVENT_BASE

namespace SST {
namespace Zodiac {

enum ZodiacEventType {
	SKIP,
	COMPUTE,
	SEND,
	RECV,
	COLLECTIVE
};

class ZodiacEvent {

	public:
		ZodiacEvent(ZodiacEventType t);
		virtual ZodiacEventType getEventType();

	protected:
		ZodiacEventType evType;

};

}
}

#endif
