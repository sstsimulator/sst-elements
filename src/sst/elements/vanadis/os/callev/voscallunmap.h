
#ifndef _H_VANADIS_SYSCALL_UNMAP
#define _H_VANADIS_SYSCALL_UNMAP

#include "os/voscallev.h"

#include <cinttypes>
#include <cstdint>

namespace SST {
namespace Vanadis {

class VanadisSyscallMemoryUnMapEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallMemoryUnMapEvent() : VanadisSyscallEvent() {}

	VanadisSyscallMemoryUnMapEvent( uint32_t core, uint32_t thr,
		uint64_t addr, uint64_t len) :
		address(addr), length(len) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_UNMAP;
	}

	uint64_t getDeallocationAddress() const { return address; }
	uint64_t getDeallocationLength() const { return length; }

private:
	uint64_t address;
	uint64_t length;

};

}
}

#endif
