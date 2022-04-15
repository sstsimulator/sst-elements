// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_FP_FPFLAGS_SET_IMM
#define _H_VANADIS_FP_FPFLAGS_SET_IMM

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

#include <type_traits>

namespace SST {
namespace Vanadis {

class VanadisFPFlagsSetImmInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPFlagsSetImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint64_t imm) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 0, 0, 0,
				isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters()),
			   imm_value(imm)
    {
		for( uint16_t i = 0; i < isa_opts->countISAFPRegisters(); ++i) {
			isa_fp_regs_in[i]  = i;
			isa_fp_regs_out[i] = i;
		}
    }

    VanadisFPFlagsSetImmInstruction*  clone() override { return new VanadisFPFlagsSetImmInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
			return "FPFLGSETI";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(buffer, buffer_size, "%s FPFLAGS <- imm: %" PRIu64 "\n",
					getInstCode(), imm_value);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
		  output->verbose(CALL_INFO, 16, 0, "Execute: 0x%llx FPFLAGS <- mask = %" PRIu64 " (0x%llx)\n",
				getInstructionAddress(), imm_value, imm_value);

		  if( (imm_value & 0x1) != 0 ) {
				fpflags.setInexact();
		  }

		  if( (imm_value & 0x2) != 0 ) {
				fpflags.setUnderflow();
		  }

		  if( (imm_value & 0x4) != 0 ) {
				fpflags.setOverflow();
		  }

		  if( (imm_value & 0x8) != 0 ) {
				fpflags.setDivZero();
		  }

		  if( (imm_value & 0x10) != 0 ) {
				fpflags.setInvalidOp();
		  }

		  update_fp_flags = true;

        markExecuted();
    }

protected:
	const uint64_t imm_value;

};

} // namespace Vanadis
} // namespace SST

#endif
