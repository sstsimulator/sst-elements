
#ifndef _H_VANADIS_BRANCH_FP
#define _H_VANADIS_BRANCH_FP

#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisBranchFPInstruction : public VanadisSpeculatedInstruction {
public:
	VanadisBranchFPInstruction(
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t cond_reg,
		const int64_t offst,
		const VanadisDelaySlotRequirement delayT) :
		VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0, 1, 0, 1, 0, delayT),
			offset(offst) {

		isa_fp_regs_in[0] = cond_reg;
	}

	VanadisBranchFPInstruction* clone() {
		return new VanadisBranchFPInstruction( *this );
	}

	virtual const char* getInstCode() const {
		return "BFP";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "BFP isa-in: %" PRIu16 ", / phys-in: %" PRIu16 " / offset: %" PRId64 "\n",
			isa_fp_regs_in[0], phys_fp_regs_in[0], offset);
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%llx) BFP isa-in: %" PRIu16 ", / phys-in: %" PRIu16 " / offset: %" PRId64 "\n",
			getInstructionAddress(), isa_fp_regs_in[0], phys_fp_regs_in[0], offset);

		const uint16_t fp_cond_reg = phys_fp_regs_in[0];
		uint32_t fp_cond_val = regFile->getFPReg<uint32_t>( fp_cond_reg );

		// is the CC code set on the compare bit in the status register?
		const bool compare_result = ((fp_cond_val & 0x800000) == 0);

		if( compare_result ) {
//			result_dir   = BRANCH_TAKEN;
			takenAddress = (uint64_t) ( ((int64_t) getInstructionAddress()) +  offset + VANADIS_SPECULATE_JUMP_ADDR_ADD );

			output->verbose(CALL_INFO, 16, 0, "-----> taken-address: 0x%llx + %" PRId64 " + %d = 0x%llx\n",
				getInstructionAddress(), offset, VANADIS_SPECULATE_JUMP_ADDR_ADD, takenAddress);
		} else {
//			result_dir = BRANCH_NOT_TAKEN;
			takenAddress = calculateStandardNotTakenAddress();

			output->verbose(CALL_INFO, 16, 0, "-----> not-taken-address: 0x%llx\n", takenAddress);
		}

		markExecuted();
	}

protected:
	const int64_t offset;

};

}
}

#endif
