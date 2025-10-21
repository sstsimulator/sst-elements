// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_NOR
#define _H_VANADIS_NOR

#include "inst/vinst.h"

#include <cstdio>
#include <cstdint>

namespace SST {
namespace Vanadis {

class VanadisNorInstruction : public VanadisInstruction
{
public:
    VanadisNorInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0)
    {

        isa_int_regs_in[0]  = src_1;
        isa_int_regs_in[1]  = src_2;
        isa_int_regs_out[0] = dest;
    }

    virtual VanadisNorInstruction* clone() override { return new VanadisNorInstruction(*this); }

    virtual VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }

    virtual const char* getInstCode() const override { return "NOR"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "NOR     %5" PRIu16 " <- %5" PRIu16 " ~| %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " ^ %5" PRIu16 ")",
            isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_in[0],
            phys_int_regs_in[1]);
    }
    virtual void instOp(VanadisRegisterFile* regFile,
                            uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0,
                            uint16_t phys_int_regs_in_1) override
    {
        uint64_t src_1 = regFile->getIntReg<uint64_t>(phys_int_regs_in_0);
        uint64_t src_2 = regFile->getIntReg<uint64_t>(phys_int_regs_in_1);

        regFile->setIntReg<uint64_t>(phys_int_regs_out_0, ~((src_1) | (src_2)));
    }
};

} // namespace Vanadis
} // namespace SST

#endif
