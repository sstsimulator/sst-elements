
#ifndef _H_VANADIS_SPECULATE
#define _H_VANADIS_SPECULATE

#include "inst/vinst.h"
#include "inst/vdelaytype.h"

namespace SST {
namespace Vanadis {

enum VanadisBranchDirection {
	BRANCH_TAKEN,
	BRANCH_NOT_TAKEN
};

const char* directionToChar( const VanadisBranchDirection dir ) {
	switch( dir ) {
	case BRANCH_TAKEN:
		return "TAKEN";
	case BRANCH_NOT_TAKEN:
		return "NOT-TAKEN";
	default:
		return "UNKNOWN";
	}
}

class VanadisSpeculatedInstruction : public VanadisInstruction {

public:
	VanadisSpeculatedInstruction(
		const uint64_t id,
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t c_phys_int_reg_in,
		const uint16_t c_phys_int_reg_out,
		const uint16_t c_isa_int_reg_in,
		const uint16_t c_isa_int_reg_out,
		const uint16_t c_phys_fp_reg_in,
		const uint16_t c_phys_fp_reg_out,
		const uint16_t c_isa_fp_reg_in,
		const uint16_t c_isa_fp_reg_out,
		const VanadisDelaySlotRequirement delayT) :
		VanadisInstruction(id, addr, hw_thr, isa_opts,
			c_phys_int_reg_in, c_phys_int_reg_out,
			c_isa_int_reg_in, c_isa_int_reg_out,
			c_phys_fp_reg_in, c_phys_fp_reg_out,
			c_isa_fp_reg_in, c_isa_fp_reg_out) {

			spec_dir   = BRANCH_NOT_TAKEN;
			result_dir = BRANCH_NOT_TAKEN;
			delayType  = delayT;

			speculatedAddress = UINT64_MAX;
		}

	virtual ~VanadisSpeculatedInstruction() {}
	VanadisSpeculatedInstruction( const VanadisSpeculatedInstruction& copy_me ) :
		VanadisInstruction(copy_me) {
		spec_dir = copy_me.spec_dir;
		result_dir = copy_me.result_dir;
		delayType = copy_me.delayType;
		speculatedAddress = copy_me.speculatedAddress;
	}

	virtual VanadisBranchDirection getSpeculatedDirection() const { return spec_dir; }
	virtual void setSpeculatedDirection( const VanadisBranchDirection dir ) { spec_dir = dir; }
	virtual uint64_t getSpeculatedAddress() { return speculatedAddress; }
	virtual void setSpeculatedAddress( const uint64_t spec_ad ) { speculatedAddress = spec_ad; }

	virtual uint64_t calculateAddress( SST::Output* output, VanadisRegisterFile* reg_file, const uint64_t current_ip ) = 0;
	virtual VanadisBranchDirection getResultDirection( const VanadisRegisterFile* reg_file ) {
		return result_dir;
	}

	virtual bool isSpeculated() const { return true; }

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_BRANCH;
	}

	virtual VanadisDelaySlotRequirement getDelaySlotType() const { return delayType; }

protected:
	uint64_t calculateStandardNotTakenAddress() {
		uint64_t new_addr = getInstructionAddress();

		switch( delayType ) {
		case VANADIS_SINGLE_DELAY_SLOT:
			new_addr += 8; break;
		case VANADIS_NO_DELAY_SLOT:
			new_addr += 4; break;
		}

		return new_addr;
	}


	VanadisBranchDirection spec_dir;
	VanadisBranchDirection result_dir;
	VanadisDelaySlotRequirement delayType;
	uint64_t speculatedAddress;

};

}
}

#endif