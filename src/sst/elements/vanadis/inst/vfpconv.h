// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_FP_CONVERT
#define _H_VANADIS_FP_CONVERT

#include "inst/vinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

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
		VanadisRegisterFormat input_f,
		VanadisRegisterFormat output_f
		 ) :
		VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0,
			( (input_f  == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1,
			( (output_f == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1,
			( (input_f  == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1,
			( (output_f == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1 ),
			input_format(input_f), output_format(output_f) {

		if( (input_f == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) {
			isa_fp_regs_in[0] = fp_src;
			isa_fp_regs_in[1] = fp_src + 1;
		} else {
			isa_fp_regs_in[0]   = fp_src;
		}

		if( (output_f == VANADIS_FORMAT_FP64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) {
			isa_fp_regs_out[0] = fp_dest;
                        isa_fp_regs_out[1] = fp_dest + 1;
		} else {
			isa_fp_regs_out[0]  = fp_dest;
		}
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

		return "FPCONVUNK";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "%s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
			getInstCode(),
			isa_fp_regs_out[0], phys_fp_regs_out[0],
			isa_fp_regs_in[0], phys_fp_regs_in[0] );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "Execute (addr=0x%llx) %s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
			getInstructionAddress(), getInstCode(),
			isa_fp_regs_out[0], phys_fp_regs_out[0],
			isa_fp_regs_in[0], phys_fp_regs_in[0] );
#endif
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
						const double fp_v = (double)( regFile->getFPReg<float>( phys_fp_regs_in[0] ) );

						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							fractureToRegisters<double>( regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], fp_v );
						} else {
							regFile->setFPReg<double>( phys_fp_regs_out[0], fp_v );
						}
					}
					break;
				case VANADIS_FORMAT_INT32:
					{
						const int32_t i_v = (int32_t)( regFile->getFPReg<float>( phys_fp_regs_in[0] ) );
						regFile->setFPReg<int32_t>( phys_fp_regs_out[0], i_v );
					}
					break;
				case VANADIS_FORMAT_INT64:
					{
						const int64_t i_v = (int64_t)( regFile->getFPReg<float>( phys_fp_regs_in[0] ) );

						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							fractureToRegisters<int64_t>( regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], i_v );
						} else {
							regFile->setFPReg<int64_t>( phys_fp_regs_out[0], i_v );
						}
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
						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							const double input_v = combineFromRegisters<double>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
							const float  fp_v    = static_cast<float>( input_v );
							regFile->setFPReg<float>( phys_fp_regs_out[0], fp_v );
						} else {
							const float fp_v = (float)( regFile->getFPReg<double>( phys_fp_regs_in[0] ) );
							regFile->setFPReg<float>( phys_fp_regs_out[0], fp_v );
						}
					}
					break;
				case VANADIS_FORMAT_FP64:
					{
						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							const double fp_v = combineFromRegisters<double>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
							fractureToRegisters<double>( regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], fp_v );
						} else {
							const double fp_v = (double)( regFile->getFPReg<double>( phys_fp_regs_in[0] ) );
							regFile->setFPReg<double>( phys_fp_regs_out[0], fp_v );
						}
					}
					break;
				case VANADIS_FORMAT_INT32:
					{
						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							const double input_v = combineFromRegisters<double>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
							const int32_t i_v    = static_cast<int32_t>( input_v );
							regFile->setFPReg<int32_t>( phys_fp_regs_out[0], i_v );
						} else {
							const int32_t i_v = (int32_t)( regFile->getFPReg<double>( phys_fp_regs_in[0] ) );
							regFile->setFPReg<int32_t>( phys_fp_regs_out[0], i_v );
						}
					}
					break;
				case VANADIS_FORMAT_INT64:
					{
						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							const double input_v = combineFromRegisters<double>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
							const int64_t i_v    = static_cast<int64_t>( input_v );
							fractureToRegisters<int64_t>( regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], i_v );
						} else {
							const int64_t i_v = (int64_t)( regFile->getFPReg<double>( phys_fp_regs_in[0] ) );
							regFile->setFPReg<int64_t>( phys_fp_regs_out[0], i_v );
						}
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
						regFile->setFPReg<float>( phys_fp_regs_out[0], fp_v );
					}
					break;
				case VANADIS_FORMAT_FP64:
					{
						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							const double fp_v = (double)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
							fractureToRegisters<double>( regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], fp_v );
						} else {
							const double fp_v = (double)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
							regFile->setFPReg<double>( phys_fp_regs_out[0], fp_v );
						}
					}
					break;
				case VANADIS_FORMAT_INT32:
					{
						const int32_t i_v = (int32_t)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
						regFile->setFPReg<int32_t>( phys_fp_regs_out[0], i_v );
					}
					break;
				case VANADIS_FORMAT_INT64:
					{
						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							const int64_t i_v = (int64_t)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
							fractureToRegisters<int64_t>( regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], i_v );
						} else {
							const int64_t i_v = (int64_t)( regFile->getFPReg<int32_t>( phys_fp_regs_in[0] ) );
							regFile->setFPReg<int64_t>( phys_fp_regs_out[0], i_v );
						}
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
						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							const int64_t input_v = combineFromRegisters<int64_t>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
							const float   fp_v    = static_cast<float>( input_v );
							regFile->setFPReg<float>( phys_fp_regs_out[0], fp_v );
						} else {
							const float fp_v = (float)( regFile->getFPReg<int64_t>( phys_fp_regs_in[0] ) );
							regFile->setFPReg<float>( phys_fp_regs_out[0], fp_v );
						}
					}
					break;
				case VANADIS_FORMAT_FP64:
					{
						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							const int64_t input_v = combineFromRegisters<int64_t>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
							const double fp_v     = static_cast<double>( input_v );
							fractureToRegisters<double>( regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], fp_v );
						} else {
							const double fp_v = (double)( regFile->getFPReg<int64_t>( phys_fp_regs_in[0] ) );
							regFile->setFPReg<double>( phys_fp_regs_out[0], fp_v );
						}
					}
					break;
				case VANADIS_FORMAT_INT32:
					{
						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							const int64_t input_v = combineFromRegisters<int64_t>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
							const int32_t i_v     = static_cast<int32_t>( input_v );
							regFile->setFPReg<int32_t>( phys_fp_regs_out[0], i_v );
						} else {
							const int32_t i_v = (int32_t)( regFile->getFPReg<int64_t>( phys_fp_regs_in[0] ) );
							regFile->setFPReg<int32_t>( phys_fp_regs_out[0], i_v );
						}
					}
					break;
				case VANADIS_FORMAT_INT64:
					{
						if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
							const int64_t input_v = combineFromRegisters<int64_t>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
							fractureToRegisters<int64_t>(regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], input_v );
						} else {
							const int64_t i_v = (int64_t)( regFile->getFPReg<int64_t>( phys_fp_regs_in[0] ) );
							regFile->setFPReg<int64_t>( phys_fp_regs_out[0], i_v );
						}
					}
					break;
				}
			}
			break;


		}

		markExecuted();
	}

protected:
	VanadisRegisterFormat input_format;
	VanadisRegisterFormat output_format;
};

}
}

#endif
