
#ifndef _H_VANADIS_OR
#define _H_VANADIS_OR

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisOrInstruction : public VanadisInstruction {
public:
	VanadisOrInstruction(
		const uint64_t id,
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src_1,
		const uint16_t src_2) :
		VanadisInstruction(id, addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_in[1]  = src_2;
		isa_int_regs_out[0] = dest;
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		return "OR";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "OR      %5" PRIu16 " <- %5" PRIu16 " | %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " | %5" PRIu16 ")",
			isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], 
			phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1] );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) OR phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16 " / in=%" PRIu16 ", %" PRIu16 "\n",
			(void*) getInstructionAddress(), phys_int_regs_out[0],
			phys_int_regs_in[0], phys_int_regs_in[1],
			isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1] );

		// If we arent targeting the "special" ISA register which ignores writes
		// then proceed with the update;
		if( phys_int_regs_out[0] != isa_options->getRegisterIgnoreWrites() ) {
			uint64_t* dest  = (uint64_t*) regFile->getIntReg( phys_int_regs_out[0] );
			uint64_t* src_1 = (uint64_t*) regFile->getIntReg( phys_int_regs_in[0]  );
			uint64_t* src_2 = (uint64_t*) regFile->getIntReg( phys_int_regs_in[1]  );

			(*dest) = (*src_1) | (*src_2);
		}

		markExecuted();
	}

};

}
}

#endif