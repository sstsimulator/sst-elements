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

#ifndef _H_VANADIS_PC_ADDI
#define _H_VANADIS_PC_ADDI

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template <typename gpr_format>
class VanadisPCAddImmInstruction : public VanadisInstruction
{
public:
    VanadisPCAddImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const gpr_format immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0),
        imm_value(immediate)
    {

        isa_int_regs_out[0] = dest;
    }

    VanadisPCAddImmInstruction* clone() override { return new VanadisPCAddImmInstruction(*this); }
    VanadisFunctionalUnitType   getInstFuncType() const override { return INST_INT_ARITH; }
    const char*                 getInstCode() const override { 
		if(sizeof(gpr_format) == 8) {
			if(std::is_signed<gpr_format>::value) {
				return "PCADDI64";
			} else {
				return "PCADDIU64";
			}
		} else {
			if(std::is_signed<gpr_format>::value) {
				return "PCADDI32";
			} else {
				return "PCADDIU32";
			}
		}
	}

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s %5" PRIu16 " <- 0x%llx + imm=%" PRId64 " (phys: %5" PRIu16 " <- 0x%llx + %" PRId64 ") = 0x%llx",
            getInstCode(), isa_int_regs_out[0], getInstructionAddress(), imm_value, phys_int_regs_out[0], getInstructionAddress(),
            imm_value, getInstructionAddress() + imm_value);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: 0x%llx %s phys: out=%" PRIu16 " in=0x%llx / imm=%" PRId64 ", isa: out=%" PRIu16
            " = 0x%llx\n",
            getInstructionAddress(), getInstCode(), phys_int_regs_out[0], getInstructionAddress(), imm_value,
            isa_int_regs_out[0], (static_cast<int64_t>(getInstructionAddress()) + imm_value));
#endif

		const gpr_format pc = static_cast<gpr_format>(getInstructionAddress());
		regFile->setIntReg<gpr_format>(phys_int_regs_out[0], pc + imm_value);

        markExecuted();
    }

private:
    const gpr_format imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
