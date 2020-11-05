
#ifndef _H_VANADIS_FP_CONVERT
#define _H_VANADIS_FP_CONVERT

#include "inst/vinst.h"
#include "inst/vfpwidth.h"

namespace SST {
namespace Vanadis {

class VanadisFPConvertInstruction : public VanadisInstruction {
public:
	VanadisFPConvertInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t fp_dest,
		const uint16_t fp_src,
		VanadisFPRegisterFormat input_f,
		VanadisFPRegisterFormat output_f
		 ) :
		VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0, 1, 1, 1, 1),
			input_format(input_f), output_format(output_f) {

		isa_fp_regs_out[0]  = fp_dest;
		isa_fp_regs_in[0]   = fp_src;
	}

	virtual VanadisFPConvertInstruction* clone() {
		return new VanadisFPConvertInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_FP_ARITH;
	}

	virtual const char* getInstCode() const {
		switch( input_format ) {
		case VANADIS_FORMAT_FP32:
			{
				switch( output_format ) {
				case VANADIS_FORMAT_FP32:
					return "F32F32CNV";
				case VANADIS_FORMAT_FP64:
					return "F32F64CNV";
				case VANADIS_FORMAT_INT32:
					return "FP32I32CNV";
				case VANADIS_FORMAT_INT64:
					return "FP32I64CNV";
				}
			}
			break;
		case VANADIS_FORMAT_FP64:
			{
				switch( output_format ) {
				case VANADIS_FORMAT_FP32:
					return "F64F32CNV";
				case VANADIS_FORMAT_FP64:
					return "F64F64CNV";
				case VANADIS_FORMAT_INT32:
					return "FP64I32CNV";
				case VANADIS_FORMAT_INT64:
					return "F64I64CNV";
				}
			}
			break;
		case VANADIS_FORMAT_INT32:
			{
				switch( output_format ) {
				case VANADIS_FORMAT_FP32:
					return "I32F32CNV";
				case VANADIS_FORMAT_FP64:
					return "I32F64CNV";
				case VANADIS_FORMAT_INT32:
					return "I32I32CNV";
				case VANADIS_FORMAT_INT64:
					return "I32I64CNV";
				}
			}
			break;
		case VANADIS_FORMAT_INT64:
			{
				switch( output_format ) {
				case VANADIS_FORMAT_FP32:
					return "I64F32CNV";
				case VANADIS_FORMAT_FP64:
					return "I64F64CNV";
				case VANADIS_FORMAT_INT32:
					return "I64I32CNV";
				case VANADIS_FORMAT_INT64:
					return "I64I64CNV";
				}
			}
			break;
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

		switch( input_format ) {

		case VANADIS_FORMAT_FP32:
			{
				switch( output_format ) {
				case VANADIS_FORMAT_FP32:
					{
						const float fp_v = (float)( regFile->getFPReg<float>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], fp_v );
					}
					break;
				case VANADIS_FORMAT_FP64:
					{
						const double fp_v = (float)( regFile->getFPReg<float>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], fp_v );
					}
					break;
				case VANADIS_FORMAT_INT32:
					{
						const int32_t i_v = (int32_t)( regFile->getFPReg<float>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], i_v );
					}
					break;
				case VANADIS_FORMAT_INT64:
					{
						const int64_t i_v = (int64_t)( regFile->getFPReg<float>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], i_v );
					}
					break;
				}
			}
			break;

		case VANADIS_FORMAT_FP64:
			{
				switch( output_format ) {
				case VANADIS_FORMAT_FP32:
					{
						const float fp_v = (float)( regFile->getFPReg<double>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], fp_v );
					}
					break;
				case VANADIS_FORMAT_FP64:
					{
						const double fp_v = (double)( regFile->getFPReg<double>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], fp_v );
					}
					break;
				case VANADIS_FORMAT_INT32:
					{
						const int32_t i_v = (int32_t)( regFile->getFPReg<double>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], i_v );
					}
					break;
				case VANADIS_FORMAT_INT64:
					{
						const int64_t i_v = (int64_t)( regFile->getFPReg<double>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], i_v );
					}
					break;
				}
			}
			break;

		case VANADIS_FORMAT_INT32:
			{
				switch( output_format ) {
				case VANADIS_FORMAT_FP32:
					{
						const float fp_v = (float)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], fp_v );
					}
					break;
				case VANADIS_FORMAT_FP64:
					{
						const double fp_v = (double)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], fp_v );
					}
					break;
				case VANADIS_FORMAT_INT32:
					{
						const int32_t i_v = (int32_t)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], i_v );
					}
					break;
				case VANADIS_FORMAT_INT64:
					{
						const int64_t i_v = (int64_t)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], i_v );
					}
					break;
				}
			}
			break;

		case VANADIS_FORMAT_INT64:
			{
				switch( output_format ) {
				case VANADIS_FORMAT_FP32:
					{
						const float fp_v = (float)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], fp_v );
					}
					break;
				case VANADIS_FORMAT_FP64:
					{
						const double fp_v = (double)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], fp_v );
					}
					break;
				case VANADIS_FORMAT_INT32:
					{
						const int32_t i_v = (int32_t)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], i_v );
					}
					break;
				case VANADIS_FORMAT_INT64:
					{
						const int64_t i_v = (int64_t)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
						regFile->setFPReg( phys_fp_regs_out[0], i_v );
					}
					break;
				}
			}
			break;


		}

		markExecuted();
	}

protected:
	VanadisFPRegisterFormat input_format;
	VanadisFPRegisterFormat output_format;
};

}
}

#endif
