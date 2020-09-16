
#ifndef _H_VANADIS_BGTZL
#define _H_VANADIS_BGTZL

#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisBranchGTZeroInstruction : public VanadisSpeculatedInstruction {
public:
	VanadisBranchGTZeroInstruction(
		const uint64_t id,
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t comp_reg,
		const uint16_t link_reg,
		const uint64_t imm_value,
		const VanadisDelaySlotRequirement delayT
		) :
		VanadisSpeculatedInstruction(id, addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0, delayT) {

		imm = (int64_t) imm_value;

		isa_int_regs_in[0] = comp_reg;
		isa_int_regs_out[0] = link_reg;

		computed_address = 0;
	}

	~VanadisBranchGTZeroInstruction() {}

	VanadisBranchGTZeroInstruction* clone() {
		return new VanadisBranchGTZeroInstruction( *this );
	}

	virtual uint64_t calculateAddress( SST::Output* output, VanadisRegisterFile* reg_file, const uint64_t current_ip ) {
		if( BRANCH_TAKEN == result_dir ) {
			const uint64_t updated_address = (uint64_t) ((int64_t) getInstructionAddress() + computed_address);

			output->verbose(CALL_INFO, 16, 0, "calculate-address: (ip): %" PRIu64 " / 0x%llx + (computed-address): %" PRId64 " / 0x%llx = %" PRIu64 " / 0x%llx\n",
				current_ip, current_ip, computed_address, computed_address, updated_address, updated_address);
			return updated_address;
		} else {
			return calculateStandardNotTakenAddress();
		}
	}

	virtual const char* getInstCode() const { return "BGTZL"; }

	virtual void printToBuffer(char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "BGTZL isa-in: %" PRIu16 " phys-in: %" PRIu16 " / isa-out: %" PRIu16 " phys-out: %" PRIu16 " imm: %" PRId64 "\n",
			isa_int_regs_in[0], phys_int_regs_in[0], isa_int_regs_out[0], phys_int_regs_out[0], imm);
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) BGTZL isa-in: %" PRIu16 " phys-in: %" PRIu16 " / isa-out: %" PRIu16 " phys-out: %" PRIu16 " imm: %" PRId64 "\n",
			getInstructionAddress(), isa_int_regs_in[0], phys_int_regs_in[0], isa_int_regs_out[0], phys_int_regs_out[0], imm);

		const int64_t comp_reg_ptr = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );
		uint64_t link_reg_ptr = 0;

		if( (comp_reg_ptr) >= 0 ) {
			// Set link address to our location + 2 instructions (this instruction, plus delay)
			regFile->setIntReg( phys_int_regs_out[0], getInstructionAddress() + 8 );
			link_reg_ptr = regFile->getIntReg<uint64_t>( phys_int_regs_out[0] );

			computed_address = 4 + imm;
			result_dir = BRANCH_TAKEN;

			output->verbose(CALL_INFO, 16, 0, "Execute result (branch-taken): %" PRId64 " >= 0, link: 0x%0llx jump-to: PC + %" PRId64 "\n",
				(comp_reg_ptr), (link_reg_ptr), computed_address);
		} else {
			computed_address = 0;
			result_dir = BRANCH_NOT_TAKEN;
			link_reg_ptr = regFile->getIntReg<uint64_t>( phys_int_regs_out[0] );

			output->verbose(CALL_INFO, 16, 0, "Execute result (branch not-taken): %" PRId64 " >= 0, link: 0x%0llx jump-to: PC + %" PRId64 "\n",
				(comp_reg_ptr), (link_reg_ptr), computed_address);
		}

		markExecuted();
	}

protected:
	int64_t imm;
	int64_t computed_address;
};

}
}

#endif
