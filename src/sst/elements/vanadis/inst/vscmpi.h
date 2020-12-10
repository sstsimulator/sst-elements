
#ifndef _H_VANADIS_SET_REG_COMPARE_IMM
#define _H_VANADIS_SET_REG_COMPARE_IMM

#include "inst/vinst.h"
#include "inst/vcmptype.h"

#include "util/vcmpop.h"

namespace SST {
namespace Vanadis {

class VanadisSetRegCompareImmInstruction : public VanadisInstruction {
public:
	VanadisSetRegCompareImmInstruction(
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src_1,
		const int64_t imm,
		const bool sgnd,
		const VanadisRegisterCompareType cType,
		const VanadisRegisterFormat fmt
		) :
		VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0 ) ,
			performSigned(sgnd), compareType(cType), imm_value(imm),
			reg_format(fmt) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_out[0] = dest;
	}

	VanadisSetRegCompareImmInstruction* clone() {
		return new VanadisSetRegCompareImmInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}
	virtual const char* getInstCode() const { return "CMPSETI"; }

	virtual void printToBuffer(char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "CMPSETI (op: %s, %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 " / phys-out: %" PRIu16 " phys-in: %" PRIu16 " / imm: %" PRId64 "\n",
			convertCompareTypeToString(compareType),
			performSigned ? "signed" : "unsigned",
			isa_int_regs_out[0], isa_int_regs_in[0],
			phys_int_regs_out[0], phys_int_regs_in[0], imm_value);
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) CMPSET (op: %s, %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 " / phys-out: %" PRIu16 " phys-in: %" PRIu16 " / imm: %" PRId64 "\n",
			getInstructionAddress(), convertCompareTypeToString(compareType),
			performSigned ? "signed" : "unsigned",
			isa_int_regs_out[0], isa_int_regs_in[0],
			phys_int_regs_out[0], phys_int_regs_in[0], imm_value);

		bool compare_result = false;

		if( performSigned ) {
			switch( reg_format ) {
			case VANADIS_FORMAT_INT64:
				{
					compare_result = registerCompareImm<int64_t>( compareType, regFile, this, output, phys_int_regs_in[0], imm_value );
				}
				break;
			case VANADIS_FORMAT_INT32:
				{
					compare_result = registerCompareImm<int32_t>( compareType, regFile, this, output, phys_int_regs_in[0], static_cast<int32_t>(imm_value) );
				}
				break;
			default:
				{
					flagError();
				}
				break;
			}
		} else {
			switch( reg_format ) {
			case VANADIS_FORMAT_INT64:
				{
					compare_result = registerCompareImm<uint64_t>( compareType, regFile, this, output, phys_int_regs_in[0], static_cast<uint64_t>(imm_value) );
				}
				break;
			case VANADIS_FORMAT_INT32:
				{
					compare_result = registerCompareImm<uint32_t>( compareType, regFile, this, output, phys_int_regs_in[0], static_cast<uint32_t>(imm_value) );
				}
				break;
			default:
				{
					flagError();
				}
				break;
			}
		}

		if( compare_result ) {
			regFile->setIntReg<uint64_t>( phys_int_regs_out[0], 1UL );
		} else {
			regFile->setIntReg<uint64_t>( phys_int_regs_out[0], 0UL );
		}

		markExecuted();
	}

protected:
	const bool performSigned;
	VanadisRegisterCompareType compareType;
	const VanadisRegisterFormat reg_format;
	const int64_t imm_value;

};

}
}

#endif
