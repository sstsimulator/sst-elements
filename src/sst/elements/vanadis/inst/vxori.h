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

#ifndef _H_VANADIS_XORI
#define _H_VANADIS_XORI

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisXorImmInstruction : public VanadisInstruction
{
public:
    VanadisXorImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint64_t immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0)
    {

        isa_int_regs_in[0]  = src_1;
        isa_int_regs_out[0] = dest;

        imm_value = immediate;
    }

    VanadisXorImmInstruction* clone() override { return new VanadisXorImmInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char*               getInstCode() const override { return "XORI"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "XORI    %5" PRIu16 " <- %5" PRIu16 " ^ imm=%" PRIu64 " (phys: %5" PRIu16 " <- %5" PRIu16 " ^ %" PRIu64
            " (0x%llx))",
            isa_int_regs_out[0], isa_int_regs_in[0], imm_value, phys_int_regs_out[0], phys_int_regs_in[0], imm_value,
            imm_value);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: (addr=%p) XORI phys: out=%" PRIu16 " in=%" PRIu16 " imm=%" PRIu64
            " / (0x%llx) , isa: out=%" PRIu16 " / in=%" PRIu16 "\n",
            (void*)getInstructionAddress(), phys_int_regs_out[0], phys_int_regs_in[0], imm_value, imm_value,
            isa_int_regs_out[0], isa_int_regs_in[0]);
#endif
        const uint64_t src_1 = regFile->getIntReg<uint64_t>(phys_int_regs_in[0]);
        regFile->setIntReg<uint64_t>(phys_int_regs_out[0], (src_1) ^ imm_value);

        markExecuted();
    }

private:
    uint64_t imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
