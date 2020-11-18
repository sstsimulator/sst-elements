
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
		mark_success = true;
	}

	VanadisSyscallResponse( int64_t ret_c ) :
		SST::Event(), return_code(ret_c) {
		mark_success = true;
	}

	~VanadisSyscallResponse() {}
	int64_t getReturnCode() const { return return_code; }
	bool isSuccessful() const { return mark_success; }
	void markFailed() { mark_success = false; }
	void markSuccessful() { mark_success = true; }
	void setSuccess( const bool succ ) { mark_success = succ; }

private:
	void serialize_order(SST::Core::Serialization::serializer &ser) override {
                Event::serialize_order(ser);
		ser & return_code;
		ser & mark_success;
	}

	ImplementSerializable( SST::Vanadis::VanadisSyscallResponse );

	int64_t return_code;
	bool    mark_success;

};

}
}

#endif
