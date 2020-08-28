
#ifndef _H_VANADIS_SET_REG_COMPARE
#define _H_VANADIS_SET_REG_COMPARE

#include "inst/vinst.h"
#include "inst/vcmptype.h"

namespace SST {
namespace Vanadis {

class VanadisSetRegCompareInstruction : public VanadisInstruction {
public:
	VanadisSetRegCompareInstruction(
		const uint64_t id,
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src_1,
		const uint16_t src_2,
		const VanadisRegisterCompareType cType
		) :
		VanadisInstruction(id, addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0 ) ,
			compareType(cType) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_in[1]  = src_2;
		isa_int_regs_out[0] = dest;
	}

	~VanadisSetRegCompareInstruction() {}

	VanadisSetRegCompareInstruction* clone() {
		return new VanadisSetRegCompareInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}
	virtual const char* getInstCode() const { return "CMPSET"; }

	virtual void printToBuffer(char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "CMPSET isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16 " / phys-out: %" PRIu16 " phys-in: %" PRIu16 ", %" PRIu16 "\n",
			isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1],
			phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1]);
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) CMPSET (op: %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16 " / phys-out: %" PRIu16 " phys-in: %" PRIu16 ", %" PRIu16 "\n",
			getInstructionAddress(), convertCompareTypeToString(compareType),
			isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1],
			phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1]);

		int64_t* reg1_ptr = (int64_t*) regFile->getIntReg( phys_int_regs_in[0] );
		int64_t* reg2_ptr = (int64_t*) regFile->getIntReg( phys_int_regs_in[1] );

		output->verbose(CALL_INFO, 16, 0, "---> reg-left: %" PRIu64 " reg-right: %" PRIu64 "\n", (*reg1_ptr), (*reg2_ptr) );

		bool compare_result = false;

		switch( compareType ) {
		case REG_COMPARE_EQ:
			{
				compare_result = (*reg1_ptr) == (*reg2_ptr);
				output->verbose(CALL_INFO, 16, 0, "-----> compare: equal     / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		case REG_COMPARE_NEQ:
			{
				compare_result = (*reg1_ptr) != (*reg2_ptr);
				output->verbose(CALL_INFO, 16, 0, "-----> compare: not-equal / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		case REG_COMPARE_LT:
			{
				compare_result = (*reg1_ptr) < (*reg2_ptr);
				output->verbose(CALL_INFO, 16, 0, "-----> compare: less-than / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		case REG_COMPARE_LTE:
			{
				compare_result = (*reg1_ptr) <= (*reg2_ptr);
				output->verbose(CALL_INFO, 16, 0, "-----> compare: less-than-eq / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		case REG_COMPARE_GT:
			{
				compare_result = (*reg1_ptr) > (*reg2_ptr);
				output->verbose(CALL_INFO, 16, 0, "-----> compare: greater-than / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		case REG_COMPARE_GTE:
			{
				compare_result = (*reg1_ptr) >= (*reg2_ptr);
				output->verbose(CALL_INFO, 16, 0, "-----> compare: greater-than-eq / result: %s\n", (compare_result ? "true" : "false") );
			}
			break;
		}

		if( compare_result ) {
			regFile->setIntReg( phys_int_regs_out[0], (uint64_t) 1 );
		} else {
			regFile->setIntReg( phys_int_regs_out[0], (uint64_t) 0 );
		}

		markExecuted();
	}

protected:
	VanadisRegisterCompareType compareType;

};

}
}

#endif
