
#ifndef _H_SHOGUN_INC_CREDIT_EVENT
#define _H_SHOGUN_INC_CREDIT_EVENT

#include <sst/core/event.h>

namespace SST {
namespace Shogun {

class ShogunCreditEvent : public SST::Event {

public:
	ShogunCreditEvent() {}
	~ShogunCreditEvent() {}

	void serialize_order(SST::Core::Serialization::serializer & ser) override {
		Event::serialize_order( ser );
        }

        ImplementSerializable(SST::Shogun::ShogunCreditEvent);

};

}
}

#endif

