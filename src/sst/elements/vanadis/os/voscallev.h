
#ifndef _H_VANADIS_SYSCALL_EVENT
#define _H_VANADIS_SYSCALL_EVENT

#include <sst/core/event.h>
#include "voscallfunc.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallEvent : public SST::Event {
public:
	VanadisSyscallEvent() :
		SST::Event() {
		core_id = 0;
		thread_id = 0;
	}

	VanadisSyscallEvent( uint32_t core, uint32_t thr ) :
		SST::Event(), core_id(core), thread_id(thr) {}

	~VanadisSyscallEvent() {}

	virtual VanadisSyscallOp getOperation() { return SYSCALL_OP_UNKNOWN; };
	uint32_t getCoreID()   const { return core_id;   }
	uint32_t getThreadID() const { return thread_id; }

private:
	void serialize_order(SST::Core::Serialization::serializer &ser) override {
        	Event::serialize_order(ser);
		ser & core_id;
		ser & thread_id;
    	}

	ImplementSerializable( SST::Vanadis::VanadisSyscallEvent );

	uint32_t core_id;
	uint32_t thread_id;
};

}
}

#endif
