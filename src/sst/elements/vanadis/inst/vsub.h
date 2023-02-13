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

#ifndef _H_VANADIS_SUB
#define _H_VANADIS_SUB

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template <typename gpr_format>
class VanadisSubInstruction : public VanadisInstruction
{
public:
    VanadisSubInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2, bool trapOverflw) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0),
        trapOverflow(trapOverflw)
    {
        isa_int_regs_in[0]  = src_1;
        isa_int_regs_in[1]  = src_2;
        isa_int_regs_out[0] = dest;
    }

    VanadisSubInstruction*    clone() override { return new VanadisSubInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char*               getInstCode() const override
    {
        if(sizeof(gpr_format) == 8) {
            return "SUB64";
        } else {
            return "SUB32";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%6s %5" PRIu16 " <- %5" PRIu16 " - %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " - %5" PRIu16 ")",
            getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0],
            phys_int_regs_in[0], phys_int_regs_in[1]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: (addr=%p) %s phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16
            " / in=%" PRIu16 ", %" PRIu16 "\n",
            (void*)getInstructionAddress(), getInstCode(), phys_int_regs_out[0], phys_int_regs_in[0],
            phys_int_regs_in[1], isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1]);
#endif
        const gpr_format src_1 = regFile->getIntReg<gpr_format>(phys_int_regs_in[0]);
        const gpr_format src_2 = regFile->getIntReg<gpr_format>(phys_int_regs_in[1]);

        const gpr_format result = src_1 - src_2;
        regFile->setIntReg<gpr_format>(phys_int_regs_out[0], result);

        markExecuted();
    }

protected:
    const bool trapOverflow;
};

} // namespace Vanadis
} // namespace SST

#endif
