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

#ifndef _H_VANADIS_FP_FPFLAGS_READ_SET
#define _H_VANADIS_FP_FPFLAGS_READ_SET

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

template<bool copy_round_mode, bool shift_round_mode, bool copy_fp_flags, bool set_fpflags>
class VanadisFPFlagsReadSetInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPFlagsReadSetInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t dest, const uint16_t src_1) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 1, 1, 1, 1,
				isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters())
    {
			isa_int_regs_in[0] = src_1;
			isa_int_regs_out[0] = dest;
    }

    VanadisFPFlagsReadSetInstruction*  clone() override { return new VanadisFPFlagsReadSetInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
			return "FPFLAGSRS";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s  %5" PRIu16 " <- %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 ")",
            getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0], phys_int_regs_out[0], phys_int_regs_in[0]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
		  update_fp_flags = true;

		  const uint64_t in_mask = regFile->getIntReg<uint64_t>(phys_int_regs_in[0]);

		  const uint64_t round_mode = copy_round_mode ? convertRoundingToInteger(fpflags.getRoundingMode()) : 0;
		  uint64_t out_mask = shift_round_mode ? (round_mode << 5) : round_mode;

		  if(copy_fp_flags) {
			  out_mask |= fpflags.inexact() ? 0x1 : 0x0;
			  out_mask |= fpflags.underflow() ? 0x2 : 0x0;
		     out_mask |= fpflags.overflow() ? 0x4 : 0x0;
   	     out_mask |= fpflags.divZero() ? 0x8 : 0x0;
	        out_mask |= fpflags.invalidOp() ? 0x10 : 0x0;
		  }

		  if(set_fpflags) {
		  if((in_mask & 0x1) == 0) {
				fpflags.clearInexact();
		  } else {
				fpflags.setInexact();
        }

        if((in_mask & 0x2) == 0) {
				fpflags.clearUnderflow();
        } else {
				fpflags.setUnderflow();
		  }

        if((in_mask & 0x4) == 0) {
				fpflags.clearOverflow();
        } else {
				fpflags.setOverflow();
        }

        if((in_mask & 0x8) == 0) {
				fpflags.clearDivZero();
        } else {
				fpflags.setDivZero();
        }

        if((in_mask & 0x10) == 0) {
				fpflags.setInvalidOp();
        } else {
				fpflags.clearInvalidOp();
        }
		  }

		  output->verbose(CALL_INFO, 16, 0, "Execute: 0x%llx set-fpflags / in-reg: %" PRIu16 " / in-mask: 0x%llx / out-reg: %" PRIu16 " / out-mask: 0x%llx\n",
				getInstructionAddress(), phys_int_regs_in[0], in_mask, phys_int_regs_out[0], out_mask);

        regFile->setIntReg<uint64_t>(phys_int_regs_out[0], out_mask);

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
