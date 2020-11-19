
#ifndef _H_VANADIS_JUMP_LINK
#define _H_VANADIS_JUMP_LINK

#include <cstdio>

#include "inst/vinst.h"
#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisJumpLinkInstruction : public VanadisSpeculatedInstruction {

public:
	VanadisJumpLinkInstruction(
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t link_reg,
		const uint64_t pc,
		const VanadisDelaySlotRequirement delayT
		) :
		VanadisSpeculatedInstruction(addr, hw_thr, isa_opts,
			0,1,0,1,0,0,0,0, delayT ) {

		isa_int_regs_out[0] = link_reg;
//		result_dir = BRANCH_TAKEN;

		takenAddress = pc;
	}

	virtual VanadisJumpLinkInstruction* clone() {
		return new VanadisJumpLinkInstruction( *this );
	}
/*
	virtual uint64_t calculateAddress( SST::Output* output, VanadisRegisterFile* reg_file, const uint64_t current_ip ) {
		output->verbose(CALL_INFO, 16, 0, "[jump-link]: jump-to: %" PRIu64 " / 0x%0llx\n", new_pc, new_pc);
		return new_pc;
	}
*/
	virtual const char* getInstCode() const {
               	return "JL";
       	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "JL      %" PRIu64 "", takenAddress );
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		const uint64_t link_value = calculateStandardNotTakenAddress();

		output->verbose(CALL_INFO, 16, 0, "Execute: JL jump-to: %" PRIu64 " / 0x%llx / link: %" PRIu16 " phys: %" PRIu16 " v: %" PRIu64 "/ 0x%llx\n",
			takenAddress, takenAddress, isa_int_regs_out[0], phys_int_regs_out[0], link_value, link_value);

		regFile->setIntReg( phys_int_regs_out[0], link_value );

		if( (takenAddress & 0x3) != 0 ) {
			flagError();
		}

		markExecuted();
	}

};

}
}

#endif
