
#ifndef _H_VANADIS_JUMP_REG_LINK
#define _H_VANADIS_JUMP_REG_LINK

#include <cstdio>

#include "inst/vinst.h"
#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisJumpRegLinkInstruction : public VanadisSpeculatedInstruction {

public:
	VanadisJumpRegLinkInstruction(
		const uint64_t id,
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t returnAddrReg,
		const uint16_t jumpToAddrReg,
		const VanadisDelaySlotRequirement delayT
		) :
		VanadisSpeculatedInstruction(id, addr, hw_thr, isa_opts,
			1,1,1,1,0,0,0,0, delayT ) {

			isa_int_regs_in[0]  = jumpToAddrReg;
			isa_int_regs_out[0] = returnAddrReg;

			// JLR means we will ALWAYS take the branch
			result_dir = BRANCH_TAKEN;
		}

		VanadisJumpRegLinkInstruction* clone() {
			return new VanadisJumpRegLinkInstruction( *this );
		}

		virtual const char* getInstCode() const {
                	return "JLR";
        	}

		virtual void printToBuffer(char* buffer, size_t buffer_size) {
			snprintf(buffer, buffer_size, "JLR     link-reg: %" PRIu16 " addr-reg: %" PRIu16 "\n",
				isa_int_regs_out[0], isa_int_regs_in[0]);
		}

		virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
			output->verbose(CALL_INFO, 16, 0, "Execute: addr=(0x%0llx) JLR isa-link: %" PRIu16 " isa-addr: %" PRIu16 " phys-link: %" PRIu16 " phys-addr: %" PRIu16 "\n",
				getInstructionAddress(), isa_int_regs_out[0], isa_int_regs_in[0], phys_int_regs_out[0], phys_int_regs_in[0]);

			const uint64_t jump_to = regFile->getIntReg<uint64_t>( phys_int_regs_in[0] );
			const uint64_t link_value = calculateStandardNotTakenAddress();

			regFile->setIntReg( phys_int_regs_out[0], link_value );

			output->verbose(CALL_INFO, 16, 0, "Execute JLR jump-to: 0x%0llx link-value: 0x%0llx\n", jump_to, link_value);

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
