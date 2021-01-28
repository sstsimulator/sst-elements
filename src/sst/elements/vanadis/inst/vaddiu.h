
#ifndef _H_VANADIS_ADDI_UNSIGNED
#define _H_VANADIS_ADDI_UNSIGNED

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisAddImmUnsignedInstruction : public VanadisInstruction {
public:
	VanadisAddImmUnsignedInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src_1,
		const uint64_t immediate,
		VanadisRegisterFormat fmt) :
		VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
			imm_value(immediate), reg_format(fmt) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_out[0] = dest;
	}

	VanadisAddImmUnsignedInstruction* clone() {
		return new VanadisAddImmUnsignedInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		return "ADDIU";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "ADDIU   %5" PRIu16 " <- %5" PRIu16 " + imm=%" PRId64 " (phys: %5" PRIu16 " <- %5" PRIu16 " + %" PRId64 ")",
			isa_int_regs_out[0], isa_int_regs_in[0], imm_value,
			phys_int_regs_out[0], phys_int_regs_in[0], imm_value );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) ADDIU phys: out=%" PRIu16 " in=%" PRIu16 " imm=%" PRId64 ", isa: out=%" PRIu16 " / in=%" PRIu16 "\n",
			(void*) getInstructionAddress(), phys_int_regs_out[0],
			phys_int_regs_in[0], imm_value,
			isa_int_regs_out[0], isa_int_regs_in[0] );
#endif
		switch( reg_format ) {
		case VANADIS_FORMAT_INT64:
			{
				const uint64_t src_1 = regFile->getIntReg<uint64_t>( phys_int_regs_in[0] );
				regFile->setIntReg<uint64_t>( phys_int_regs_out[0], src_1 + imm_value, false );
			}
			break;
		case VANADIS_FORMAT_INT32:
			{
				const uint32_t src_1 = regFile->getIntReg<uint32_t>( phys_int_regs_in[0] );
                                regFile->setIntReg<uint32_t>( phys_int_regs_out[0], src_1 + imm_value, false );
			}
			break;
		default:
			{
				flagError();
			}
			break;
		}
		markExecuted();
	}

private:
	VanadisRegisterFormat reg_format;
	const uint64_t imm_value;

};

}
}

#endif
