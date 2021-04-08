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

#ifndef _H_VANADIS_FP_SUB
#define _H_VANADIS_FP_SUB

#include "inst/vinst.h"
#include "inst/vregfmt.h"
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
		const VanadisRegisterFormat input_f) :
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
		switch( input_format ) {
                case VANADIS_FORMAT_FP64:
                        return "FP64SUB";
                case VANADIS_FORMAT_FP32:
                        return "FP32SUB";
                case VANADIS_FORMAT_INT64:
                        return "FPINT64SUB";
                case VANADIS_FORMAT_INT32:
                        return "FPINT32SUB";
                }

		return "FPUNK";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "FPSUB   %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 ")",
			isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1],
			phys_fp_regs_out[0], phys_fp_regs_in[0], phys_fp_regs_in[1] );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
#ifdef VANADIS_BUILD_DEBUG
		char* int_register_buffer = new char[256];
                char* fp_register_buffer = new char[256];

                writeIntRegs( int_register_buffer, 256 );
                writeFPRegs(  fp_register_buffer,  256 );

                output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%llx) %s int: %s / fp: %s\n",
                        getInstructionAddress(), getInstCode(),
                        int_register_buffer, fp_register_buffer);

		delete[] int_register_buffer;
                delete[] fp_register_buffer;
#endif
		switch( input_format ) {
		case VANADIS_FORMAT_FP32:
			{
				const float src_1 = regFile->getFPReg<float>( phys_fp_regs_in[0] );
				const float src_2 = regFile->getFPReg<float>( phys_fp_regs_in[1] );

				output->verbose(CALL_INFO, 16, 0, "---> %f + %f = %f\n", src_1, src_2, (src_1 - src_2));

				regFile->setFPReg<float>( phys_fp_regs_out[0], ((src_1) - (src_2)));
			}
			break;
		case VANADIS_FORMAT_FP64:
			{
				if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
					const double src_1 = combineFromRegisters<double>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
					const double src_2 = combineFromRegisters<double>( regFile, phys_fp_regs_in[2], phys_fp_regs_in[3] );

					output->verbose(CALL_INFO, 16, 0, "---> %f + %f = %f\n", src_1, src_2, (src_1 - src_2));

					fractureToRegisters<double>( regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], src_1 - src_2 );
				} else {
					const double src_1 = regFile->getFPReg<double>( phys_fp_regs_in[0] );
					const double src_2 = regFile->getFPReg<double>( phys_fp_regs_in[1] );

					output->verbose(CALL_INFO, 16, 0, "---> %f + %f = %f\n", src_1, src_2, (src_1 - src_2));

					regFile->setFPReg<double>( phys_fp_regs_out[0], src_1 - src_2 );
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
	VanadisRegisterFormat input_format;

};

}
}

#endif
