
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

			spec = BRANCH_TAKEN;
			delayType = delayT;
		}

	virtual ~VanadisSpeculatedInstruction() {}

	virtual VanadisBranchDirection getSpeculatedDirection() const { return spec; }
	virtual void setSpeculatedDirection( const VanadisBranchDirection dir ) { spec = dir; }

	virtual uint64_t getSpeculatePCAddress( const VanadisRegisterFile* reg_file ) = 0;
	virtual VanadisBranchDirection getResultDirection( const VanadisRegisterFile* reg_file ) {
		return BRANCH_TAKEN;
	}

	virtual bool isSpeculated() const { return true; }

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_BRANCH;
	}

	virtual VanadisDelaySlotRequirement getDelaySlotType() const { return delayType; }

protected:
	VanadisBranchDirection spec;
	VanadisDelaySlotRequirement delayType;

};

}
}

#endif
