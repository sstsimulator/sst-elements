
#ifndef _H_VANADIS_SYSCALL_UNAME
#define _H_VANADIS_SYSCALL_UNAME

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallUnameEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallUnameEvent() : VanadisSyscallEvent() {}
	VanadisSyscallUnameEvent( uint32_t core, uint32_t thr, uint64_t uname_info_adr ) :
		VanadisSyscallEvent(core, thr), uname_info_addr(uname_info_adr) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_UNAME;
	}

	uint64_t getUnameInfoAddress() const {
		return uname_info_addr;
	}

private:
	uint64_t uname_info_addr;

};

}
}

#endif
