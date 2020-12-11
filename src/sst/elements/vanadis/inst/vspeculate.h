
#ifndef _H_VANADIS_SPECULATE
#define _H_VANADIS_SPECULATE

#include "inst/vinst.h"
#include "inst/vdelaytype.h"

#define VANADIS_SPECULATE_JUMP_ADDR_ADD 4

namespace SST {
namespace Vanadis {

class VanadisSpeculatedInstruction : public VanadisInstruction {

public:
	VanadisSpeculatedInstruction(
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
		VanadisInstruction(addr, hw_thr, isa_opts,
			c_phys_int_reg_in, c_phys_int_reg_out,
			c_isa_int_reg_in, c_isa_int_reg_out,
			c_phys_fp_reg_in, c_phys_fp_reg_out,
			c_isa_fp_reg_in, c_isa_fp_reg_out) {

			delayType  = delayT;

			// default speculated, branch not-taken address

			if( delayT == VANADIS_NO_DELAY_SLOT ) {
				speculatedAddress = getInstructionAddress() + 4;
			} else {
				speculatedAddress = getInstructionAddress() + 8;
			}

			// speculatedAddress = (addr + 4);
			takenAddress      = UINT64_MAX;
		}

/*
	VanadisSpeculatedInstruction( const VanadisSpeculatedInstruction& copy_me ) :
		VanadisInstruction(copy_me) {

		delayType  = copy_me.delayType;

		speculatedAddress = copy_me.speculatedAddress;
		takenAddress      = copy_me.takenAddress;
	}
*/

	virtual uint64_t getSpeculatedAddress() const { return speculatedAddress; }
	virtual void setSpeculatedAddress( const uint64_t spec_ad ) { speculatedAddress = spec_ad; }
	virtual uint64_t getTakenAddress() const { return takenAddress; }
	virtual bool isSpeculated() const { return true; }

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_BRANCH;
	}

	virtual VanadisDelaySlotRequirement getDelaySlotType() const { return delayType; }

protected:
	uint64_t calculateStandardNotTakenAddress() {
		uint64_t new_addr = getInstructionAddress();

		switch( delayType ) {
		case VANADIS_CONDITIONAL_SINGLE_DELAY_SLOT:
		case VANADIS_SINGLE_DELAY_SLOT:
			new_addr += 8; break;
		case VANADIS_NO_DELAY_SLOT:
			new_addr += 4; break;
		}

		return new_addr;
	}

	VanadisDelaySlotRequirement delayType;
	uint64_t speculatedAddress;
	uint64_t takenAddress;

};

}
}

#endif
