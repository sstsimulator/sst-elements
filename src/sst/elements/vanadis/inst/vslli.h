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

#ifndef _H_VANADIS_SLLI
#define _H_VANADIS_SLLI

#include "inst/vinst.h"
#include "inst/vregfmt.h"

namespace SST {
namespace Vanadis {

class VanadisShiftLeftLogicalImmInstruction : public VanadisInstruction {
public:
	VanadisShiftLeftLogicalImmInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src_1,
		const uint64_t immediate,
		VanadisRegisterFormat fmt) :
		VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
			reg_format(fmt) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_out[0] = dest;

		imm_value = immediate;
	}

	VanadisShiftLeftLogicalImmInstruction* clone() {
		return new VanadisShiftLeftLogicalImmInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		return "SLLI";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "SLLI    %5" PRIu16 " <- %5" PRIu16 " << imm=%" PRId64 " (phys: %5" PRIu16 " <- %5" PRIu16 " << %" PRId64 ")",
			isa_int_regs_out[0], isa_int_regs_in[0], imm_value,
			phys_int_regs_out[0], phys_int_regs_in[0], imm_value );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) SLLI phys: out=%" PRIu16 " in=%" PRIu16 " imm=%" PRId64 ", isa: out=%" PRIu16 " / in=%" PRIu16 "\n",
			(void*) getInstructionAddress(), phys_int_regs_out[0],
			phys_int_regs_in[0], imm_value,
			isa_int_regs_out[0], isa_int_regs_in[0] );
#endif
		assert( imm_value > 0 );

		switch( reg_format ) {
		case VANADIS_FORMAT_INT64:
			{
				const uint64_t src_1 = regFile->getIntReg<uint64_t>( phys_int_regs_in[0] );
				regFile->setIntReg<uint64_t>( phys_int_regs_out[0], src_1 << imm_value );
			}
			break;
		case VANADIS_FORMAT_INT32:
			{
				const uint32_t src_1 = regFile->getIntReg<uint32_t>( phys_int_regs_in[0] );
                                regFile->setIntReg<uint32_t>( phys_int_regs_out[0], src_1 << static_cast<uint32_t>(imm_value) );
			}
			break;
		case VANADIS_FORMAT_FP32:
		case VANADIS_FORMAT_FP64:
			{
				flagError();
			}
		}

		markExecuted();
	}

private:
	VanadisRegisterFormat reg_format;
	uint64_t imm_value;

};

}
}

#endif
