
#ifndef _H_VANADIS_SETREG
#define _H_VANADIS_SETREG

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisSetRegisterInstruction : public VanadisInstruction {
public:
	VanadisSetRegisterInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const int64_t immediate,
		VanadisRegisterFormat fmt) :
		VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0),
			reg_format(fmt) {

		isa_int_regs_out[0] = dest;
		imm_value = immediate;
	}

	VanadisSetRegisterInstruction* clone() {
		return new VanadisSetRegisterInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		return "SETREG";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "SETREG  %5" PRIu16 " <- imm=%" PRId64 " (phys: %5" PRIu16 " <- %" PRId64 ")",
			isa_int_regs_out[0], imm_value, phys_int_regs_out[0], imm_value );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) SETREG phys: out=%" PRIu16 " imm=%" PRId64 ", isa: out=%" PRIu16 "\n",
			getInstructionAddress(), phys_int_regs_out[0], imm_value, isa_int_regs_out[0] );

		switch( reg_format ) {
		case VANADIS_FORMAT_INT64:
			{
				regFile->setIntReg<int64_t>( phys_int_regs_out[0], imm_value );
			}
			break;
		case VANADIS_FORMAT_INT32:
			{
				regFile->setIntReg<int32_t>( phys_int_regs_out[0], static_cast<int32_t>(imm_value) );
			}
			break;
		default:
			{
				flagError();
			}
			break;
		}

		output->verbose(CALL_INFO, 16, 0, "Result-reg %" PRIu16 ": %" PRId64 "\n",
			phys_int_regs_out[0], imm_value);

		markExecuted();
	}

private:
	VanadisRegisterFormat reg_format;
	int64_t imm_value;

};

}
}

#endif
