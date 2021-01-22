
#ifndef _H_VANADIS_ADD
#define _H_VANADIS_ADD

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisAddInstruction : public VanadisInstruction {
public:
	VanadisAddInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src_1,
		const uint16_t src_2,
		const bool signd,
		VanadisRegisterFormat fmt) :
		VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0),
			perform_signed(signd), reg_format(fmt) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_in[1]  = src_2;
		isa_int_regs_out[0] = dest;
	}

	VanadisAddInstruction* clone() {
		return new VanadisAddInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		if( perform_signed ) {
			return "ADD";
		} else {
			return "ADDU";
		}
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "%s    %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 ")",
			getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1],
			phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1] );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) %s phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16 " / in=%" PRIu16 ", %" PRIu16 "\n",
			(void*) getInstructionAddress(), getInstCode(),
			phys_int_regs_out[0],
			phys_int_regs_in[0], phys_int_regs_in[1],
			isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1] );
#endif

		switch( reg_format ) {
		case VANADIS_FORMAT_INT64:
			{
				if( perform_signed ) {
					const int64_t src_1 = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );
					const int64_t src_2 = regFile->getIntReg<int64_t>( phys_int_regs_in[1] );

					regFile->setIntReg<int64_t>( phys_int_regs_out[0], ((src_1) + (src_2)));
				} else {
					const uint64_t src_1 = regFile->getIntReg<uint64_t>( phys_int_regs_in[0] );
					const uint64_t src_2 = regFile->getIntReg<uint64_t>( phys_int_regs_in[1] );

					regFile->setIntReg<uint64_t>( phys_int_regs_out[0], ((src_1) + (src_2)));
				}
			}
			break;
		case VANADIS_FORMAT_INT32:
			{
				if( perform_signed ) {
					const int32_t src_1 = regFile->getIntReg<int32_t>( phys_int_regs_in[0] );
					const int32_t src_2 = regFile->getIntReg<int32_t>( phys_int_regs_in[1] );

					regFile->setIntReg<int32_t>( phys_int_regs_out[0], ((src_1) + (src_2)));
				} else {
					const uint32_t src_1 = regFile->getIntReg<uint32_t>( phys_int_regs_in[0] );
					const uint32_t src_2 = regFile->getIntReg<uint32_t>( phys_int_regs_in[1] );

					regFile->setIntReg<uint32_t>( phys_int_regs_out[0], ((src_1) + (src_2)));
				}
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
	bool perform_signed;
	VanadisRegisterFormat reg_format;

};

}
}

#endif
