
#ifndef _H_VANADIS_JUMP_REG
#define _H_VANADIS_JUMP_REG

#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisJumpRegInstruction : public VanadisSpeculatedInstruction {
public:
	VanadisJumpRegInstruction(
		const uint64_t id,
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t jump_to_reg) :
		VanadisSpeculatedInstruction(id, addr, hw_thr, isa_opts, 1, 0, 1, 0,
			0, 0, 0, 0, VANADIS_SINGLE_DELAY_SLOT) {

		isa_int_regs_in[0] = jump_to_reg;
		result_dir = BRANCH_TAKEN;
	}

	~VanadisJumpRegInstruction() {}

	VanadisJumpRegInstruction* clone() {
		return new VanadisJumpRegInstruction( *this );
	}

	virtual uint64_t calculateAddress( SST::Output* output, VanadisRegisterFile* reg_file, const uint64_t current_ip ) {
		uint64_t jump_to = 0;
		reg_file->getIntReg( phys_int_regs_in[0], &jump_to );

		// Last two bits MUST be zero
		if( (jump_to & 0x3) == 0 ) {
			output->verbose(CALL_INFO, 16, 0, "[jump]: jump-to: 0x%0llx\n", jump_to);
		} else {
			output->fatal(CALL_INFO, -1, "[flag-error]: flagging error for JR instruction, jump address (0x%0llx) is not naturally aligned.\n",
				jump_to);
			flagError();
		}

		return jump_to;
	}

	virtual const char* getInstCode() const { return "JR"; }

	virtual void printToBuffer(char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "JR   isa-in: %" PRIu16 " / phys-in: %" PRIu16 "\n",
			isa_int_regs_in[0], phys_int_regs_in[0] );
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) JR   isa-in: %" PRIu16 " / phys-in: %" PRIu16 "\n",
			getInstructionAddress(), isa_int_regs_in[0], phys_int_regs_in[0] );
		markExecuted();
	}

};

}
}

#endif
