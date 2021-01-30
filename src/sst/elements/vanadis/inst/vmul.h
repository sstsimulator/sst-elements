
#ifndef _H_VANADIS_MUL
#define _H_VANADIS_MUL

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisMultiplyInstruction : public VanadisInstruction {
public:
	VanadisMultiplyInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src_1,
		const uint16_t src_2,
		VanadisRegisterFormat fmt) :
		VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0),
			reg_format(fmt) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_in[1]  = src_2;
		isa_int_regs_out[0] = dest;
	}

	VanadisMultiplyInstruction* clone() {
		return new VanadisMultiplyInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		return "MUL";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "MUL     %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 ")",
			isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1],
			phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1] );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) MUL phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16 " / in=%" PRIu16 ", %" PRIu16 "\n",
			(void*) getInstructionAddress(), phys_int_regs_out[0],
			phys_int_regs_in[0], phys_int_regs_in[1],
			isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1] );
#endif
		switch( reg_format ) {
		case VANADIS_FORMAT_INT64:
			{
				const int64_t src_1 = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );
				const int64_t src_2 = regFile->getIntReg<int64_t>( phys_int_regs_in[1] );

				regFile->setIntReg<int64_t>( phys_int_regs_out[0], (src_1) * (src_2) );
			}
			break;
		case VANADIS_FORMAT_INT32:
			{
				const int32_t src_1 = regFile->getIntReg<int32_t>( phys_int_regs_in[0] );
				const int32_t src_2 = regFile->getIntReg<int32_t>( phys_int_regs_in[1] );

				regFile->setIntReg<int32_t>( phys_int_regs_out[0], (src_1) * (src_2) );
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

protected:
	VanadisRegisterFormat reg_format;

};

}
}

#endif
