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

#ifndef _H_VANADIS_FP_2_FP
#define _H_VANADIS_FP_2_FP

#include "inst/vinst.h"
#include "inst/vregfmt.h"

#include "util/vfpreghandler.h"

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
		VanadisRegisterFormat fp_w
		) :
		VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0,
			( (fp_w == VANADIS_FORMAT_FP64 || fp_w == VANADIS_FORMAT_INT64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1,
			( (fp_w == VANADIS_FORMAT_FP64 || fp_w == VANADIS_FORMAT_INT64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1,
			( (fp_w == VANADIS_FORMAT_FP64 || fp_w == VANADIS_FORMAT_INT64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1,
			( (fp_w == VANADIS_FORMAT_FP64 || fp_w == VANADIS_FORMAT_INT64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1),
			move_width(fp_w) {

		if( (fp_w == VANADIS_FORMAT_FP64 || fp_w == VANADIS_FORMAT_INT64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) {
			isa_fp_regs_out[0] = fp_dest;
			isa_fp_regs_out[1] = fp_dest + 1;
			isa_fp_regs_in[0]  = fp_src;
			isa_fp_regs_in[1]  = fp_src + 1;
		} else {
			isa_fp_regs_out[0]  = fp_dest;
			isa_fp_regs_in[0]   = fp_src;
		}
	}

	virtual VanadisFP2FPInstruction* clone() {
		return new VanadisFP2FPInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_FP_ARITH;
	}

	virtual const char* getInstCode() const {
		switch( move_width ) {
		case VANADIS_FORMAT_INT32:
		case VANADIS_FORMAT_FP32:
			return "FP2FP32";
		case VANADIS_FORMAT_INT64:
		case VANADIS_FORMAT_FP64:
			return "FP2FP64";
		}

		return "FPUNK";
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
		switch( move_width ) {
		case VANADIS_FORMAT_INT32:
		case VANADIS_FORMAT_FP32:
			{
				const int32_t fp_v = regFile->getFPReg<int32_t>( phys_fp_regs_in[0] );
				regFile->setFPReg<int32_t>( phys_fp_regs_out[0], fp_v );
			}
			break;
		case VANADIS_FORMAT_INT64:
		case VANADIS_FORMAT_FP64:
			{
				if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
					const int32_t v_0 = regFile->getFPReg<int32_t>( phys_fp_regs_in[0] );
					regFile->setFPReg<int32_t>( phys_fp_regs_out[0], v_0 );

					const int32_t v_1 = regFile->getFPReg<int32_t>( phys_fp_regs_in[1] );
					regFile->setFPReg<int32_t>( phys_fp_regs_out[1], v_1 );
				} else {
					const int64_t fp_v = regFile->getFPReg<int64_t>( phys_fp_regs_in[0] );
					regFile->setFPReg<int64_t>( phys_fp_regs_out[0], fp_v );
				}
			}
			break;
		}

		markExecuted();
	}

protected:
	VanadisRegisterFormat move_width;

};

}
}

#endif
