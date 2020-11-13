
#ifndef _H_VANADIS_SET_REG_COMPARE_IMM
#define _H_VANADIS_SET_REG_COMPARE_IMM

#include "inst/vinst.h"
#include "inst/vcmptype.h"

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
		const VanadisRegisterCompareType cType
		) :
		VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0 ) ,
			performSigned(sgnd), compareType(cType), imm_value(imm) {

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
			const int64_t reg1_ptr = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );
			output->verbose(CALL_INFO, 16, 0, "---> reg-left: %" PRId64 " imm: %" PRId64 "\n", (reg1_ptr), imm_value );

			switch( compareType ) {
			case REG_COMPARE_EQ:
				{
					compare_result = (reg1_ptr) == imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: equal     / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			case REG_COMPARE_NEQ:
				{
					compare_result = (reg1_ptr) != imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: not-equal / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			case REG_COMPARE_LT:
				{
					compare_result = (reg1_ptr) < imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: less-than / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			case REG_COMPARE_LTE:
				{
					compare_result = (reg1_ptr) <= imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: less-than-eq / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			case REG_COMPARE_GT:
				{
					compare_result = (reg1_ptr) > imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: greater-than / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			case REG_COMPARE_GTE:
				{
					compare_result = (reg1_ptr) >= imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: greater-than-eq / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			}
		} else {
			const uint64_t reg1_ptr = regFile->getIntReg<uint64_t>( phys_int_regs_in[0] );
			output->verbose(CALL_INFO, 16, 0, "---> reg-left: %" PRIu64 " imm: %" PRId64 "\n", (reg1_ptr), imm_value );

			switch( compareType ) {
			case REG_COMPARE_EQ:
				{
					compare_result = (reg1_ptr) == imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: equal     / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			case REG_COMPARE_NEQ:
				{
					compare_result = (reg1_ptr) != imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: not-equal / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			case REG_COMPARE_LT:
				{
					compare_result = (reg1_ptr) < imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: less-than / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			case REG_COMPARE_LTE:
				{
					compare_result = (reg1_ptr) <= imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: less-than-eq / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			case REG_COMPARE_GT:
				{
					compare_result = (reg1_ptr) > imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: greater-than / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			case REG_COMPARE_GTE:
				{
					compare_result = (reg1_ptr) >= imm_value;
					output->verbose(CALL_INFO, 16, 0, "-----> compare: greater-than-eq / result: %s\n", (compare_result ? "true" : "false") );
				}
				break;
			}
		}

		if( compare_result ) {
			regFile->setIntReg( phys_int_regs_out[0], (uint64_t) 1 );
		} else {
			regFile->setIntReg( phys_int_regs_out[0], (uint64_t) 0 );
		}

		markExecuted();
	}

protected:
	const bool performSigned;
	VanadisRegisterCompareType compareType;
	const int64_t imm_value;

};

}
}

#endif
