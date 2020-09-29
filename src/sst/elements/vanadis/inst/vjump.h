
#ifndef _H_VANADIS_JUMP
#define _H_VANADIS_JUMP

#include <cstdio>

#include "inst/vinst.h"
#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisJumpInstruction : public VanadisSpeculatedInstruction {

public:
	VanadisJumpInstruction(
		const uint64_t id,
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint64_t pc,
		const VanadisDelaySlotRequirement delayT
		) :
		VanadisSpeculatedInstruction(id, addr, hw_thr, isa_opts,
			0,0,0,0,0,0,0,0, delayT ) {

		result_dir = BRANCH_TAKEN;
		takenAddress = pc;
	}

	virtual VanadisJumpInstruction* clone() {
		return new VanadisJumpInstruction( *this );
	}
/*
	virtual uint64_t calculateAddress( SST::Output* output, VanadisRegisterFile* reg_file, const uint64_t current_ip ) {
		output->verbose(CALL_INFO, 16, 0, "[jump]: jump-to: %" PRIu64 " / 0x%0llx\n", new_pc, new_pc);
		return new_pc;
	}
*/
	virtual const char* getInstCode() const {
               	return "JMP";
       	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "JUMP    %" PRIu64 "", takenAddress);
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		if( (takenAddress & 0x3) != 0 ) {
			flagError();
		}

		markExecuted();
	}

};

}
}

#endif
