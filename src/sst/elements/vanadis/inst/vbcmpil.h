
#ifndef _H_VANADIS_BRANCH_REG_COMPARE_IMM_LINK
#define _H_VANADIS_BRANCH_REG_COMPARE_IMM_LINK

#include "inst/vspeculate.h"
#include "inst/vcmptype.h"

namespace SST {
namespace Vanadis {

class VanadisBranchRegCompareImmLinkInstruction : public VanadisSpeculatedInstruction {
public:
	VanadisBranchRegCompareImmLinkInstruction(
		const uint64_t id,
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t src_1,
		const int64_t imm,
		const int64_t offst,
		const uint16_t link_reg,
		const VanadisDelaySlotRequirement delayT,
		const VanadisRegisterCompareType cType
		) :
		VanadisSpeculatedInstruction(id, addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0, delayT),
			compareType(cType), imm_value(imm), offset(offst) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_out[0] = link_reg;
	}

	~VanadisBranchRegCompareImmLinkInstruction() {}

	VanadisBranchRegCompareImmLinkInstruction* clone() {
		return new VanadisBranchRegCompareImmLinkInstruction( *this );
	}

	virtual uint64_t calculateAddress( SST::Output* output, VanadisRegisterFile* reg_file, const uint64_t current_ip ) {
		if( result_dir == BRANCH_TAKEN ) {
			const uint64_t updated_address = (uint64_t) ((int64_t) getInstructionAddress() + 4 + offset);

			output->verbose(CALL_INFO, 16, 0, "calculate-address: (taken) (ip): %" PRIu64" / 0x%llx + (4 + offset: %" PRId64 " / 0x%llx = %" PRIu64 " / 0x%llx\n",
				current_ip, current_ip, offset, offset, updated_address, updated_address);
			return updated_address;
		} else {
			const uint64_t updated_address = calculateStandardNotTakenAddress();
			output->verbose(CALL_INFO, 16, 0, "calculate-address: (not-taken) ins-addr: 0x%0llx, result: 0x%0llx\n",
				getInstructionAddress(), updated_address);
			return updated_address;
		}
	}

	virtual const char* getInstCode() const { return "BCMPIL"; }

	virtual void printToBuffer(char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "BCMPIL isa-in: %" PRIu16 " / phys-in: %" PRIu16 " / imm: %" PRId64 " / offset: %" PRId64 " / isa-link: %" PRIu16 " / phys-link: %" PRIu16 "\n",
			isa_int_regs_in[0], phys_int_regs_in[0], imm_value, offset, isa_int_regs_out[0], phys_int_regs_out[0]);
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) BCMPIL isa-in: %" PRIu16 " / phys-in: %" PRIu16 " / imm: %" PRId64 " / offset: %" PRId64 " / isa-link: %" PRIu16 " / phys-link: %" PRIu16 "\n",
			getInstructionAddress(), isa_int_regs_in[0], phys_int_regs_in[0], imm_value, offset, isa_int_regs_out[0], phys_int_regs_out[0] );

		int64_t reg_1 = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );

		output->verbose(CALL_INFO, 16, 0, "---> reg-left: %" PRId64 " imm: %" PRId64 "\n", (reg_1), imm_value );

		bool compare_result = false;

		switch( compareType ) {
		case REG_COMPARE_EQ:
			{
				compare_result = (reg_1) == imm_value;
				output->verbose(CALL_INFO, 16, 0, "-----> compare: equal     / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		case REG_COMPARE_NEQ:
			{
				compare_result = (reg_1) != imm_value;
				output->verbose(CALL_INFO, 16, 0, "-----> compare: not-equal / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		case REG_COMPARE_LT:
			{
				compare_result = (reg_1) < imm_value;
				output->verbose(CALL_INFO, 16, 0, "-----> compare: less-than / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		case REG_COMPARE_LTE:
			{
				compare_result = (reg_1) <= imm_value;
				output->verbose(CALL_INFO, 16, 0, "-----> compare: less-than-eq / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		case REG_COMPARE_GT:
			{
				compare_result = (reg_1) > imm_value;
				output->verbose(CALL_INFO, 16, 0, "-----> compare: greater-than / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		case REG_COMPARE_GTE:
			{
				compare_result = (reg_1) >= imm_value;
				output->verbose(CALL_INFO, 16, 0, "-----> compare: greater-than-eq / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		}

		if( compare_result ) {
			result_dir = BRANCH_TAKEN;

			// Update the link address
			const uint64_t update_address = (uint64_t) ((int64_t) getInstructionAddress() + 4 + offset);
			regFile->setIntReg( phys_int_regs_out[0], update_address );
		} else {
			result_dir = BRANCH_NOT_TAKEN;
		}

		markExecuted();
	}

protected:
	int64_t offset;
	int64_t imm_value;
	VanadisRegisterCompareType compareType;

};

}
}

#endif
