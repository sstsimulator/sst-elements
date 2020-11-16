
#ifndef _H_VANADIS_FP_SUB
#define _H_VANADIS_FP_SUB

#include "inst/vinst.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

class VanadisFPSubInstruction : public VanadisInstruction {
public:
	VanadisFPSubInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src_1,
		const uint16_t src_2,
		const VanadisFPRegisterFormat input_f) :
		VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0,
			( (input_f == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 4 : 2,
			( (input_f == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1,
			( (input_f == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 4 : 2,
                        ( (input_f == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1),
			input_format(input_f) {

		if( (input_f == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) {
			isa_fp_regs_in[0]  = src_1;
			isa_fp_regs_in[1]  = src_1 + 1;
			isa_fp_regs_in[2]  = src_2;
			isa_fp_regs_in[3]  = src_2 + 1;
			isa_fp_regs_out[0] = dest;
			isa_fp_regs_out[1] = dest + 1;
		} else {
			isa_fp_regs_in[0]  = src_1;
			isa_fp_regs_in[1]  = src_2;
			isa_fp_regs_out[0] = dest;
		}
	}

	VanadisFPSubInstruction* clone() {
		return new VanadisFPSubInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_FP_ARITH;
	}

	virtual const char* getInstCode() const {
		return "FPSUB";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "FPSUB   %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 ")",
			isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1],
			phys_fp_regs_out[0], phys_fp_regs_in[0], phys_fp_regs_in[1] );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) FPSUB phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16 " / in=%" PRIu16 ", %" PRIu16 "\n",
			(void*) getInstructionAddress(), phys_fp_regs_out[0],
			phys_fp_regs_in[0], phys_fp_regs_in[1],
			isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1] );

		switch( input_format ) {
		case VANADIS_FORMAT_FP32:
			{
				const float src_1 = regFile->getFPReg<float>( phys_fp_regs_in[0] );
				const float src_2 = regFile->getFPReg<float>( phys_fp_regs_in[1] );

				regFile->setFPReg( phys_fp_regs_out[0], ((src_1) - (src_2)));
			}
			break;
		case VANADIS_FORMAT_FP64:
			{
				if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
					const double src_1 = combineFromRegisters<double>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
					const double src_2 = combineFromRegisters<double>( regFile, phys_fp_regs_in[2], phys_fp_regs_in[3] );

					fractureToRegisters<double>( regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], src_1 - src_2 );
				} else {
					const double src_1 = regFile->getFPReg<double>( phys_fp_regs_in[0] );
					const double src_2 = regFile->getFPReg<double>( phys_fp_regs_in[1] );

					regFile->setFPReg( phys_fp_regs_out[0], src_1 - src_2 );
				}
			}
			break;
		default:
			{
				output->verbose(CALL_INFO, 16, 0, "Unknown floating point type.\n");
				flagError();
			}
			break;
		}


		markExecuted();
	}

protected:
	VanadisFPRegisterFormat input_format;

};

}
}

#endif
