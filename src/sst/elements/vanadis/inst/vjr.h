
#ifndef _H_VANADIS_JUMP_REG
#define _H_VANADIS_JUMP_REG

#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisJumpRegInstruction : public VanadisSpeculatedInstruction {
public:
	VanadisJumpRegInstruction(
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t jump_to_reg,
		const VanadisDelaySlotRequirement delayT ) :
		VanadisSpeculatedInstruction(addr, hw_thr, isa_opts,
			1, 0, 1, 0, 0, 0, 0, 0, delayT ) {

		isa_int_regs_in[0] = jump_to_reg;
	}

	VanadisJumpRegInstruction* clone() {
		return new VanadisJumpRegInstruction( *this );
	}

	virtual const char* getInstCode() const { return "JR"; }

	virtual void printToBuffer(char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "JR   isa-in: %" PRIu16 " / phys-in: %" PRIu16 "\n",
			isa_int_regs_in[0], phys_int_regs_in[0] );
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) JR   isa-in: %" PRIu16 " / phys-in: %" PRIu16 "\n",
			getInstructionAddress(), isa_int_regs_in[0], phys_int_regs_in[0] );
#endif
		takenAddress = regFile->getIntReg<uint64_t>( phys_int_regs_in[0] );

		if( (takenAddress & 0x3) != 0 ) {
			flagError();
		}

		markExecuted();
	}

};

}
}

#endif
