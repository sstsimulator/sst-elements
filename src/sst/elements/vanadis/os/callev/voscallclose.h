
#ifndef _H_VANADIS_SYSCALL_CLOSE
#define _H_VANADIS_SYSCALL_CLOSE

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallCloseEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallCloseEvent() : VanadisSyscallEvent() {}
	VanadisSyscallCloseEvent( uint32_t core, uint32_t thr, int32_t file_id ) :
		VanadisSyscallEvent(core, thr), close_file_descriptor(file_id) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_CLOSE;
	}

	int32_t getFileDescriptor() const {
		return close_file_descriptor;
	}

private:
	int32_t   close_file_descriptor;

};

}
}

#endif
