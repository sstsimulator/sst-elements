
#ifndef _H_SHOGUN_INIT_EVENT
#define _H_SHOGUN_INIT_EVENT

#include <sst/core/event.h>

namespace SST {
namespace Shogun {

class ShogunInitEvent : public SST::Event {

public:
	ShogunInitEvent() :
		input_port_count(0),
		output_port_count(0) {}

	ShogunInitEvent(const int inputPortCount,
		const int outputPortCount) :
		input_port_count(inputPortCount),
		output_port_count(outputPortCount) {}

	~ShogunInitEvent() {}

	int getInputPortCount() const {
		return input_port_count;
	}

	int getOutputPortCount() const {
		return output_port_count;
	}

	void serialize_order(SST::Core::Serialization::serializer & ser) override {
		Event::serialize_order( ser );
		ser & input_port_count;
		ser & output_port_count;
        }

	ImplementSerializable(SST::Shogun::ShogunInitEvent);

private:
	int input_port_count;
	int output_port_count;

};

}
}

#endif

