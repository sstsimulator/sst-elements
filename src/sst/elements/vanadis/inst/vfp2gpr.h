
#ifndef _H_VANADIS_FP_2_GPR
#define _H_VANADIS_FP_2_GPR

#include "inst/vinst.h"
#include "inst/vfpwidth.h"

namespace SST {
namespace Vanadis {

class VanadisFP2GPRInstruction : public VanadisInstruction {
public:
	VanadisFP2GPRInstruction(
		const uint64_t id,
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t int_dest,
		const uint16_t fp_src,
		VanadisFPWidth fp_w
		 ) :
		VanadisInstruction(id, addr, hw_thr, isa_opts, 0, 1, 0, 1, 1, 0, 1, 0),
			move_width(fp_w) {

		isa_int_regs_out[0]  = int_dest;
		isa_fp_regs_in[0]   = fp_src;
	}

	virtual VanadisFP2GPRInstruction* clone() {
		return new VanadisFP2GPRInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		switch( move_width ) {
		case VANADIS_WIDTH_F32:
			return "FP2GPR32";
		case VANADIS_WIDTH_F64:
			return "FP2GPR64";
		}
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "%s int-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
			getInstCode(),
			isa_int_regs_out[0], phys_int_regs_out[0],
			isa_fp_regs_in[0], phys_fp_regs_in[0] );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute (addr=0x%llx) %s int-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
			getInstructionAddress(), getInstCode(),
			isa_int_regs_out[0], phys_int_regs_out[0],
			isa_fp_regs_in[0], phys_fp_regs_in[0] );

		switch( move_width ) {
		case VANADIS_WIDTH_F32:
			{
				const uint32_t fp_v = regFile->getFPReg<uint32_t>( phys_fp_regs_in[0] );
				regFile->setIntReg( phys_int_regs_out[0], fp_v );
			}
			break;
		case VANADIS_WIDTH_F64:
			{
				const uint64_t fp_v = regFile->getFPReg<uint64_t>( phys_fp_regs_in[0] );
				regFile->setIntReg( phys_int_regs_out[0], fp_v );
			}
			break;
		}

		markExecuted();
	}

protected:
	VanadisFPWidth move_width;

};

}
}

#endif
