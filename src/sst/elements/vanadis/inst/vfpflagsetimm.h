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

#ifndef _H_VANADIS_FP_FPFLAGS_READ_SET_IMM
#define _H_VANADIS_FP_FPFLAGS_READ_SET_IMM

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

template<bool copy_round_mode, bool set_fpflags>
class VanadisFPFlagsReadSetImmInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPFlagsReadSetImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t dest, const uint64_t imm) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 1, 0, 1,
				isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters()),
				imm_value(imm)
    {
			isa_int_regs_out[0] = dest;
    }

    VanadisFPFlagsReadSetImmInstruction*  clone() override { return new VanadisFPFlagsReadSetImmInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
			return "FPFLAGSRSI";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s  %5" PRIu16 " <- %" PRIu64 " (phys: %5" PRIu16 " <- %" PRIu64 ")",
            getInstCode(), isa_int_regs_out[0], imm_value, phys_int_regs_out[0], imm_value);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
		  update_fp_flags = true;

		  uint64_t out_mask = copy_round_mode ? (convertRoundingToInteger(fpflags.getRoundingMode()) << 5) : 0;

		  out_mask |= fpflags.inexact() ? 0x1 : 0x0;
		  out_mask |= fpflags.underflow() ? 0x2 : 0x0;
	     out_mask |= fpflags.overflow() ? 0x4 : 0x0;
        out_mask |= fpflags.divZero() ? 0x8 : 0x0;
        out_mask |= fpflags.invalidOp() ? 0x10 : 0x0;

		  if(set_fpflags) {
		  if((imm_value & 0x1) == 0) {
				fpflags.clearInexact();
		  } else {
				fpflags.setInexact();
        }

        if((imm_value & 0x2) == 0) {
				fpflags.clearUnderflow();
        } else {
				fpflags.setUnderflow();
		  }

        if((imm_value & 0x4) == 0) {
				fpflags.clearOverflow();
        } else {
				fpflags.setOverflow();
        }

        if((imm_value & 0x8) == 0) {
				fpflags.clearDivZero();
        } else {
				fpflags.setDivZero();
        }

        if((imm_value & 0x10) == 0) {
				fpflags.setInvalidOp();
        } else {
				fpflags.clearInvalidOp();
        }
		  }

		  output->verbose(CALL_INFO, 16, 0, "Execute: 0x%llx set-fpflags / in-reg: %" PRIu16 " / in-mask: 0x%llx / out-reg: %" PRIu16 " / out-mask: 0x%llx\n",
				getInstructionAddress(), phys_int_regs_in[0], imm_value, phys_int_regs_out[0], out_mask);

        regFile->setIntReg<uint64_t>(phys_int_regs_out[0], out_mask);

        markExecuted();
    }

protected:
	const uint64_t imm_value;

};

} // namespace Vanadis
} // namespace SST

#endif
