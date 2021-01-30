
#ifndef _H_VANADIS_BRANCH_REG_COMPARE_IMM
#define _H_VANADIS_BRANCH_REG_COMPARE_IMM

#include "inst/vspeculate.h"
#include "inst/vcmptype.h"

#include "util/vcmpop.h"

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
		const VanadisRegisterCompareType cType,
		const VanadisRegisterFormat fmt
		) :
		VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, 1, 0, 1, 0, 0, 0, 0, 0, delayT),
			compareType(cType), imm_value(imm), offset(offst), reg_format(fmt) {

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
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) BCMPI isa-in: %" PRIu16 " / phys-in: %" PRIu16 " / imm: %" PRId64 " / offset: %" PRId64 "\n",
			getInstructionAddress(), isa_int_regs_in[0], phys_int_regs_in[0], imm_value, offset );
#endif
		bool compare_result = false;

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
	VanadisRegisterFormat reg_format;

};

}
}

#endif
