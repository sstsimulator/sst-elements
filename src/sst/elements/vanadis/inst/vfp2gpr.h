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

#ifndef _H_VANADIS_FP_2_GPR
#define _H_VANADIS_FP_2_GPR

#include "inst/vinst.h"
#include "inst/vregfmt.h"

#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

class VanadisFP2GPRInstruction : public VanadisInstruction {
public:
	VanadisFP2GPRInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t int_dest,
		const uint16_t fp_src,
		VanadisRegisterFormat fp_w
		 ) :
		VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1,
			( (fp_w == VANADIS_FORMAT_FP64 || fp_w == VANADIS_FORMAT_INT64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1,
			0,
			( (fp_w == VANADIS_FORMAT_FP64 || fp_w == VANADIS_FORMAT_INT64) && ( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ? 2 : 1,
			0),
			move_width(fp_w) {

		isa_int_regs_out[0]  = int_dest;

		if( ( (fp_w == VANADIS_FORMAT_FP64 || fp_w == VANADIS_FORMAT_INT64) &&
			( VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode() ) ) ) {
			isa_fp_regs_in[0]   = fp_src;
			isa_fp_regs_in[1]   = fp_src + 1;
		} else {
			isa_fp_regs_in[0]   = fp_src;
		}
	}

	virtual VanadisFP2GPRInstruction* clone() {
		return new VanadisFP2GPRInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		switch( move_width ) {
		case VANADIS_FORMAT_INT32:
		case VANADIS_FORMAT_FP32:
			return "FP2GPR32";
		case VANADIS_FORMAT_INT64:
		case VANADIS_FORMAT_FP64:
			return "FP2GPR64";
		}

		return "FPGPRUNK";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "%s int-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
			getInstCode(),
			isa_int_regs_out[0], phys_int_regs_out[0],
			isa_fp_regs_in[0], phys_fp_regs_in[0] );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "Execute (addr=0x%llx) %s int-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
			getInstructionAddress(), getInstCode(),
			isa_int_regs_out[0], phys_int_regs_out[0],
			isa_fp_regs_in[0], phys_fp_regs_in[0] );
#endif
		switch( move_width ) {
		case VANADIS_FORMAT_INT32:
		case VANADIS_FORMAT_FP32:
			{
				const int32_t fp_v = regFile->getFPReg<int32_t>( phys_fp_regs_in[0] );
				regFile->setIntReg<int32_t>( phys_int_regs_out[0], fp_v );
			}
			break;
		case VANADIS_FORMAT_INT64:
		case VANADIS_FORMAT_FP64:
			{
				if( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
					const int64_t fp_v = combineFromRegisters<int64_t>( regFile, phys_fp_regs_in[0], phys_fp_regs_in[1] );
					regFile->setIntReg<int64_t>( phys_int_regs_out[0], fp_v );
				} else {
					const int64_t fp_v = regFile->getFPReg<int64_t>( phys_fp_regs_in[0] );
					regFile->setIntReg<int64_t>( phys_int_regs_out[0], fp_v );
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
