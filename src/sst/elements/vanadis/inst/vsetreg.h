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

#ifndef _H_VANADIS_SETREG
#define _H_VANADIS_SETREG

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template<typename reg_format>
class VanadisSetRegisterInstruction : public VanadisInstruction
{
public:
    VanadisSetRegisterInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const reg_format immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0)
    {

        isa_int_regs_out[0] = dest;
        imm_value           = immediate;
    }

    VanadisSetRegisterInstruction* clone() override { return new VanadisSetRegisterInstruction(*this); }
    VanadisFunctionalUnitType      getInstFuncType() const override { return INST_INT_ARITH; }
    const char*                    getInstCode() const override { return "SETREG"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size, "SETREG  %5" PRIu16 " <- imm=%" PRId64 " (phys: %5" PRIu16 " <- %" PRId64 ")",
            isa_int_regs_out[0], imm_value, phys_int_regs_out[0], imm_value);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= 16) {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: (addr=0x%0llx) SETREG phys: out=%" PRIu16 " imm=%" PRId64 ", isa: out=%" PRIu16 "\n",
                getInstructionAddress(), phys_int_regs_out[0], imm_value, isa_int_regs_out[0]);
        }
#endif

		regFile->setIntReg<reg_format>(phys_int_regs_out[0], imm_value);

#ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= 16) {
            output->verbose(CALL_INFO, 16, 0, "Result-reg %" PRIu16 ": %" PRId64 "\n", phys_int_regs_out[0], imm_value);
        }
#endif

        markExecuted();
    }

private:
    reg_format imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
