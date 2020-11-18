
#ifndef _H_VANADIS_EXIT_RESPONSE
#define _H_VANADIS_EXIT_RESPONSE

#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisExitResponse : public SST::Event {

public:
	VanadisExitResponse() : SST::Event(), return_code(0) {}
	VanadisExitResponse( int64_t rc ) : SST::Event(), return_code(rc) {}
	~VanadisExitResponse() {}

	int64_t getReturnCode() const { return return_code; }

private:
	void serialize_order( SST::Core::Serialization::serializer& ser ) override {
		Event::serialize_order(ser);
		ser & return_code;
	}

	ImplementSerializable( SST::Vanadis::VanadisExitResponse );

	int64_t return_code;
};

}
}

#endif
