
#ifndef _H_VANADIS_SYSCALL
#define _H_VANADIS_SYSCALL

#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisSysCallInstruction : public VanadisInstruction {
public:
	VanadisSysCallInstruction(
		const uint64_t id,
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts) :
			VanadisInstruction(id, addr, hw_thr, isa_opts,
			isa_opts->countISAIntRegisters(),
			isa_opts->countISAIntRegisters(),
			isa_opts->countISAIntRegisters(),
			isa_opts->countISAIntRegisters(),
			isa_opts->countISAFPRegisters(),
			isa_opts->countISAFPRegisters(),
			isa_opts->countISAFPRegisters(),
			isa_opts->countISAFPRegisters() ) {

		for( uint16_t i = 0; i < isa_opts->countISAIntRegisters(); ++i ) {
			isa_int_regs_in[i] = i;
			isa_int_regs_out[i] = i;
		}

		for( uint16_t i = 0; i < isa_opts->countISAFPRegisters(); ++i ) {
			isa_fp_regs_in[i] = i;
			isa_fp_regs_out[i] = i;
		}
	}

	~VanadisSysCallInstruction() {}

	VanadisSysCallInstruction* clone() {
		return new VanadisSysCallInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_SYSCALL;
	}

	virtual const char* getInstCode() const { return "SYSCALL"; }

	virtual void printToBuffer(char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "SYSCALL (syscall-code-isa: %" PRIu16 ", phys: %" PRIu16 ")\n",
			isa_options->getISASysCallCodeReg(),
			phys_int_regs_in[ isa_options->getISASysCallCodeReg() ] );
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		const uint64_t code_reg_ptr = regFile->getIntReg<uint64_t>( isa_options->getISASysCallCodeReg() );
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) SYSCALL (isa: %" PRIu16 ", os-code: %" PRIu64 ")\n",
			getInstructionAddress(), isa_options->getISASysCallCodeReg(),  code_reg_ptr );
		markExecuted();
	}

};

}
}

#endif
