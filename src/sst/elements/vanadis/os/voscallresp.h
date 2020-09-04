
#ifndef _H_VANADIS_SYSCALL_RESPONSE
#define _H_VANADIS_SYSCALL_RESPONSE

#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisSyscallResponse : public SST::Event {
public:
	VanadisSyscallResponse() :
		SST::Event() {
		return_code = 0;
	}

	VanadisSyscallResponse( int64_t ret_c ) :
		SST::Event(), return_code(ret_c) {
	}

	~VanadisSyscallResponse() {}
	int64_t getReturnCode() { return return_code; }

private:
	void serialize_order(SST::Core::Serialization::serializer &ser) override {
                Event::serialize_order(ser);
		ser & return_code;
	}

	ImplementSerializable( SST::Vanadis::VanadisSyscallResponse );
	int64_t return_code;

};

}
}

#endif
