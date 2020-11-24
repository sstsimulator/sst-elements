
#ifndef _H_VANADIS_BRANCH_REG_COMPARE_IMM
#define _H_VANADIS_BRANCH_REG_COMPARE_IMM

#include "inst/vspeculate.h"
#include "inst/vcmptype.h"

namespace SST {
namespace Vanadis {

class VanadisBranchRegCompareImmInstruction : public VanadisSpeculatedInstruction {
public:
	VanadisBranchRegCompareImmInstruction(
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t src_1,
		const int64_t imm,
		const int64_t offst,
		const VanadisDelaySlotRequirement delayT,
		const VanadisRegisterCompareType cType
		) :
		VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, 1, 0, 1, 0, 0, 0, 0, 0, delayT),
			compareType(cType), imm_value(imm), offset(offst) {

		isa_int_regs_in[0] = src_1;
	}

	VanadisBranchRegCompareImmInstruction* clone() {
		return new VanadisBranchRegCompareImmInstruction( *this );
	}

	virtual const char* getInstCode() const { return "BCMPI"; }

	virtual void printToBuffer(char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "BCMPI isa-in: %" PRIu16 " / phys-in: %" PRIu16 " / imm: %" PRId64 " / offset: %" PRId64 "\n",
			isa_int_regs_in[0], phys_int_regs_in[0], imm_value, offset);
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) BCMPI isa-in: %" PRIu16 " / phys-in: %" PRIu16 " / imm: %" PRId64 " / offset: %" PRId64 "\n",
			getInstructionAddress(), isa_int_regs_in[0], phys_int_regs_in[0], imm_value, offset );

		const int64_t reg1_ptr = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );

		output->verbose(CALL_INFO, 16, 0, "---> reg-left: %" PRId64 " imm: %" PRId64 "\n", (reg1_ptr), imm_value );

		bool compare_result = false;

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
		default:
			{
				output->fatal(CALL_INFO, -1, "Unknown comparison operation. Halt.\n");
				compare_result = false;
			}
			break;
		}

		if( compare_result ) {
			const int64_t instruction_address = getInstructionAddress();
			const int64_t ins_addr_and_offset = instruction_address + offset + VANADIS_SPECULATE_JUMP_ADDR_ADD;

			takenAddress = static_cast<uint64_t>( ins_addr_and_offset );
		} else {
			takenAddress = calculateStandardNotTakenAddress();
		}

		markExecuted();
	}

protected:
	const int64_t offset;
	const int64_t imm_value;
	VanadisRegisterCompareType compareType;

};

}
}

#endif
