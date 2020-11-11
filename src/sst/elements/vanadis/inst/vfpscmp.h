
#ifndef _H_VANADIS_FP_SET_REG_COMPARE
#define _H_VANADIS_FP_SET_REG_COMPARE

#include "inst/vinst.h"
#include "inst/vcmptype.h"

#define VANADIS_MIPS_FP_COMPARE_BIT 0x800000
#define VANADIS_MIPS_FP_COMPARE_BIT_INVERSE 0xFF7FFFFF

namespace SST {
namespace Vanadis {

class VanadisFPSetRegCompareInstruction : public VanadisInstruction {
public:
	VanadisFPSetRegCompareInstruction(
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src_1,
		const uint16_t src_2,
		const VanadisFPRegisterFormat r_fmt,
		const VanadisRegisterCompareType cType
		) :
		VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0, 3, 1, 3, 1 ) ,
			reg_fmt(r_fmt), compareType(cType) {

		isa_fp_regs_in[0]  = src_1;
		isa_fp_regs_in[1]  = src_2;
		isa_fp_regs_in[2]  = dest;
		isa_fp_regs_out[0] = dest;
	}

	VanadisFPSetRegCompareInstruction* clone() {
		return new VanadisFPSetRegCompareInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_FP_ARITH;
	}
	virtual const char* getInstCode() const { return "FPCMPST"; }

	virtual void printToBuffer(char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "FPCMPST (op: %s, %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16 " / phys-out: %" PRIu16 " phys-in: %" PRIu16 ", %" PRIu16 "\n",
			convertCompareTypeToString(compareType),
			registerFormatToString(reg_fmt),
			isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1],
			phys_fp_regs_out[0], phys_fp_regs_in[0], phys_fp_regs_in[1]);
	}

	template<typename T>
	bool performCompare( SST::Output* output, VanadisRegisterFile* regFile, uint16_t left, uint16_t right ) {
		const T left_value  = regFile->getFPReg<T>( left  );
		const T right_value = regFile->getFPReg<T>( right );

		switch( compareType ) {
		case REG_COMPARE_EQ:
			return (left_value == right_value);
		case REG_COMPARE_NEQ:
			return (left_value != right_value);
		case REG_COMPARE_LT:
			return (left_value < right_value);
		case REG_COMPARE_LTE:
			return (left_value <= right_value);
		case REG_COMPARE_GT:
			return (left_value > right_value);
		case REG_COMPARE_GTE:
			return (left_value >= right_value);
		default:
			output->fatal(CALL_INFO, -1, "Unknown compare type.\n");
			return false;
		}
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) FPCMPST (op: %s, %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16 " / phys-out: %" PRIu16 " phys-in: %" PRIu16 ", %" PRIu16 "\n",
			getInstructionAddress(), convertCompareTypeToString(compareType),
			registerFormatToString(reg_fmt),
			isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1],
			phys_fp_regs_out[0], phys_fp_regs_in[0], phys_fp_regs_in[1]);

		bool compare_result = false;

		switch( reg_fmt ) {
		case VANADIS_FORMAT_FP32:
			compare_result = performCompare<float>( output, regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
			break;
		case VANADIS_FORMAT_FP64:
			compare_result = performCompare<double>( output, regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
			break;
		case VANADIS_FORMAT_INT32:
			compare_result = performCompare<int32_t>( output, regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
			break;
		case VANADIS_FORMAT_INT64:
			compare_result = performCompare<int64_t>( output, regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
			break;
		default:
			output->fatal(CALL_INFO, -1, "Unknown data format type.\n");
		}

		const uint16_t cond_reg_in  = phys_fp_regs_in[2];
		const uint16_t cond_reg_out = phys_fp_regs_out[0];
		uint32_t cond_val = (regFile->getFPReg<uint32_t>( cond_reg_in ) & VANADIS_MIPS_FP_COMPARE_BIT_INVERSE);

		if( compare_result ) {
			cond_val = (cond_val | VANADIS_MIPS_FP_COMPARE_BIT);
			output->verbose(CALL_INFO, 16, 0, "---> result: true\n");
		} else {
			output->verbose(CALL_INFO, 16, 0, "---> result: false\n");
		}

		regFile->setFPReg<uint32_t>( cond_reg_out, cond_val );

		markExecuted();
	}

protected:
	VanadisFPRegisterFormat reg_fmt;
	VanadisRegisterCompareType compareType;

};

}
}

#endif
