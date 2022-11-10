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

#ifndef _H_VANADIS_FP_FPFLAGS_SET
#define _H_VANADIS_FP_FPFLAGS_SET

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

#include <type_traits>

namespace SST {
namespace Vanadis {

class VanadisFPFlagsSetInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPFlagsSetInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t src_1) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 1, 0, 1, 0, 0, 0, 0, 0)
    {
		isa_int_regs_in[0] = src_1;
    }

    VanadisFPFlagsSetInstruction*  clone() override { return new VanadisFPFlagsSetInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
			return "FPFLGSET";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(buffer, buffer_size, "%s FPFLAGS <- %" PRIu16 " (phys: %" PRIu16 ")\n",
					getInstCode(), isa_int_regs_in[0], phys_int_regs_in[0]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
		if(checkFrontOfROB()) {
			const uint64_t mask_in = regFile->getIntReg<uint64_t>(phys_int_regs_in[0]);

			if(output->getVerboseLevel() >= 16) {
				output->verbose(CALL_INFO, 16, 0, "Execute: 0x%llx %s in-reg: %" PRIu16 " / phys: %" PRIu16 " -> mask = %" PRIu64 " (0x%llx)\n",
					getInstructionAddress(), getInstCode(), isa_int_regs_in[0], phys_int_regs_in[0], mask_in, mask_in);
			}

			if( (mask_in & 0x1) != 0 ) {
				fpflags.setInexact();
			}

			if( (mask_in & 0x2) != 0 ) {
				fpflags.setUnderflow();
			}

			if( (mask_in & 0x4) != 0 ) {
				fpflags.setOverflow();
			}

			if( (mask_in & 0x8) != 0 ) {
				fpflags.setDivZero();
			}

			if( (mask_in & 0x10) != 0 ) {
				fpflags.setInvalidOp();
			}

			set_fp_flags = true;

			markExecuted();
		} else {
			output->verbose(CALL_INFO, 16, 0, "not front of ROB for ins: 0x%llx %s\n", getInstructionAddress(), getInstCode());
		}
    }
};

} // namespace Vanadis
} // namespace SST

#endif
