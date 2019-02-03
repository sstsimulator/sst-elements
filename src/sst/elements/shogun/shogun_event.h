
#ifndef _H_SHOGUN_EVENT
#define _H_SHOGUN_EVENT

#include <sst/core/event.h>

namespace SST {
namespace Shogun {

class ShogunEvent : public SST::Event {

public:
	ShogunEvent(int dst) :
		dest(dst) {
		src = -1;
		innerEvent = nullptr;
	}

	ShogunEvent(int dst, int source) :
		dest(dst), src(source) {
		innerEvent = nullptr;
	}

	ShogunEvent() :
		dest(-1), src(-1) {
		innerEvent = nullptr;
	}

	~ShogunEvent() {}

	int getDestination() const {
		return dest;
	}

	int getSource() const {
		return src;
	}

	void setSource(int source) {
		src = source;
	}

	SST::Event* getInnerEvent() {
		return innerEvent;
	}

	void setEventPayload( SST::Event* payload ) {
		innerEvent = payload;
	}

        void serialize_order(SST::Core::Serialization::serializer & ser) override {
                Event::serialize_order( ser );
                ser & dest;
		ser & src;

		//TODO: serialize innerEvent
        }

        ImplementSerializable(SST::Shogun::ShogunEvent);

private:
	int dest;
	int src;
	SST::Event* innerEvent;

};

}
}

#endif
