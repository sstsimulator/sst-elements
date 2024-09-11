// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_AND
#define _H_VANADIS_AND

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisAndInstruction : public virtual VanadisInstruction
{
public:
    VanadisAndInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0)
    {

        isa_int_regs_in[0]  = src_1;
        isa_int_regs_in[1]  = src_2;
        isa_int_regs_out[0] = dest;
    }

    VanadisAndInstruction*    clone() override { return new VanadisAndInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char*               getInstCode() const override { return "AND"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "AND     %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 ")",
            isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_in[0],
            phys_int_regs_in[1]);
    }

    void instOp(VanadisRegisterFile* regFile, uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0,
                uint16_t phys_int_regs_in_1) override
    {
        const uint64_t src_1 = regFile->getIntReg<uint64_t>(phys_int_regs_in_0);
        const uint64_t src_2 = regFile->getIntReg<uint64_t>(phys_int_regs_in_1);

        regFile->setIntReg<uint64_t>(phys_int_regs_out_0, (src_1 & src_2));
    }

    // void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    // {
    //     uint16_t phys_int_regs_out_0 = phys_int_regs_out[0];
    //     uint16_t phys_int_regs_in_0 = phys_int_regs_in[0];
    //     uint16_t phys_int_regs_in_1 = phys_int_regs_in[1];
    //     log(output, 16, 65535, phys_int_regs_out_0,phys_int_regs_in_0,
    //             phys_int_regs_in_1);
    //     instOp(regFile, phys_int_regs_out_0,phys_int_regs_in_0,
    //             phys_int_regs_in_1);
    //     markExecuted();
    // }
};

class VanadisSIMTAndInstruction : public VanadisSIMTInstruction, public VanadisAndInstruction
{
public:
    VanadisSIMTAndInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0),
        VanadisSIMTInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0),
        VanadisAndInstruction(addr, hw_thr, isa_opts, dest, src_1, src_2)
    {

        // isa_int_regs_in[0]  = src_1;
        // isa_int_regs_in[1]  = src_2;
        // isa_int_regs_out[0] = dest;
        ;
    }

    VanadisSIMTAndInstruction*    clone() override { return new VanadisSIMTAndInstruction(*this); }


    // void simtExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    // {
    //     uint16_t phys_int_regs_out_0 =getPhysIntRegOut(0, VanadisSIMTInstruction::sw_thread);
    //     uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0, VanadisSIMTInstruction::sw_thread);
    //     uint16_t phys_int_regs_in_1 = getPhysIntRegIn(1, VanadisSIMTInstruction::sw_thread);
    //     this->log(output, 16, VanadisSIMTInstruction::sw_thread, phys_int_regs_out_0,phys_int_regs_in_0,
    //             phys_int_regs_in_1);
    //     this->instOp(regFile, phys_int_regs_out_0,phys_int_regs_in_0,
    //             phys_int_regs_in_1);
    // }
};

} // namespace Vanadis
} // namespace SST

#endif
