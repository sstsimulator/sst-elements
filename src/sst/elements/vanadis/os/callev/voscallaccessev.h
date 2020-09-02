
#ifndef _H_VANADIS_SYSCALL_ACCESS
#define _H_VANADIS_SYSCALL_ACCESS

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallAccessEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallAccessEvent() : VanadisSyscallEvent() {}
	VanadisSyscallAccessEvent( uint32_t core, uint32_t thr ) :
		VanadisSyscallEvent(core, thr) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_ACCESS;
	}

};

}
}

#endif
