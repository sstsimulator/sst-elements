

namespace SST {
namespace Zodiac {

class ZodiacEvent {

	enum ZodiacEventType {
		SKIP,
		COMPUTE,
		SEND,
		RECV,
		COLLECTIVE
	};

	public:
		ZodiacEvent(ZodiacEventType t);
		virtual ZodiacEventType getEventType();

	protected:
		ZodiacEventType evType;

};

}
}
