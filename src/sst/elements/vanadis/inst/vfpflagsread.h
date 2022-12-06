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

#ifndef _H_VANADIS_FP_FPFLAGS_READ
#define _H_VANADIS_FP_FPFLAGS_READ

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

#include <type_traits>

namespace SST {
namespace Vanadis {

template<bool copy_round_mode, bool shift_round_mode, bool copy_fp_flags>
class VanadisFPFlagsReadInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPFlagsReadInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t dest) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 1, 0, 1,
				isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters())
    {
			static_assert( !((copy_round_mode && (!shift_round_mode)) && copy_fp_flags), "Cannot copy round, not shift and copy FP flags\n");

			isa_int_regs_out[0] = dest;

			for( uint16_t i = 0; i < isa_opts->countISAFPRegisters(); ++i) {
         	isa_fp_regs_in[i]  = i;
         	isa_fp_regs_out[i] = i;
      	}
    }

    VanadisFPFlagsReadInstruction*  clone() override { return new VanadisFPFlagsReadInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
			return "FPFLGRD";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(buffer, buffer_size, "%s %" PRIu16 " <- FPFLAGS (phys: %" PRIu16 ")\n",
					getInstCode(), isa_int_regs_out[0], phys_int_regs_out[0]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
		  uint64_t flags_out = 0;

        if(copy_round_mode) {
			  	flags_out = convertRoundingToInteger(fpflags.getRoundingMode());

				if(shift_round_mode) {
					flags_out <<= 5;
				}
	      }

		  if(copy_fp_flags) {
			  flags_out |= fpflags.inexact() ? 0x1 : 0x0;
			  flags_out |= fpflags.underflow() ? 0x2 : 0x0;
		     flags_out |= fpflags.overflow() ? 0x4 : 0x0;
   	     flags_out |= fpflags.divZero() ? 0x8 : 0x0;
	        flags_out |= fpflags.invalidOp() ? 0x10 : 0x0;
		  }

		  output->verbose(CALL_INFO, 16, 0, "Execute: 0x%llx out-reg: %" PRIu16 " / out-mask: 0x%llx / copy_round: %c / shift_round: %c / copy_fp: %c\n",
				getInstructionAddress(), phys_int_regs_out[0], flags_out,
				copy_round_mode ? 'y' : 'n', shift_round_mode ? 'y' : 'n', copy_fp_flags ? 'y' : 'n');

        regFile->setIntReg<uint64_t>(phys_int_regs_out[0], flags_out);

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
