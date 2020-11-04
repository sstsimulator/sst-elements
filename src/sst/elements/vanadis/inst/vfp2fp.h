
#ifndef _H_VANADIS_FP_2_FP
#define _H_VANADIS_FP_2_FP

#include "inst/vinst.h"
#include "inst/vfpwidth.h"

namespace SST {
namespace Vanadis {

class VanadisFP2FPInstruction : public VanadisInstruction {
public:
	VanadisFP2FPInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t fp_dest,
		const uint16_t fp_src,
		VanadisFPWidth fp_w
		 ) :
		VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0, 1, 1, 1, 1),
			move_width(fp_w) {

		isa_fp_regs_out[0]  = fp_dest;
		isa_fp_regs_in[0]   = fp_src;
	}

	virtual VanadisFP2FPInstruction* clone() {
		return new VanadisFP2FPInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_FP_ARITH;
	}

	virtual const char* getInstCode() const {
		switch( move_width ) {
		case VANADIS_WIDTH_F32:
			return "FP2FP32";
		case VANADIS_WIDTH_F64:
			return "FP2FP64";
		}
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "%s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
			getInstCode(),
			isa_fp_regs_out[0], phys_fp_regs_out[0],
			isa_fp_regs_in[0], phys_fp_regs_in[0] );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute (addr=0x%llx) %s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
			getInstructionAddress(), getInstCode(),
			isa_fp_regs_out[0], phys_fp_regs_out[0],
			isa_fp_regs_in[0], phys_fp_regs_in[0] );

		switch( move_width ) {
		case VANADIS_WIDTH_F32:
			{
				const float fp_v = regFile->getFPReg<float>( phys_fp_regs_in[0] );
				regFile->setFPReg( phys_fp_regs_out[0], fp_v );
			}
			break;
		case VANADIS_WIDTH_F64:
			{
				const double fp_v = regFile->getFPReg<double>( phys_fp_regs_in[0] );
				regFile->setFPReg( phys_fp_regs_out[0], fp_v );
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
