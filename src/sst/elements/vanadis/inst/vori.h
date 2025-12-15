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

#ifndef _H_VANADIS_ORI
#define _H_VANADIS_ORI

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisOrImmInstruction : public virtual VanadisInstruction
{
public:
    VanadisOrImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint64_t immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0)
    {

        isa_int_regs_in[0]  = src_1;
        isa_int_regs_out[0] = dest;

        imm_value = immediate;
    }

    VanadisOrImmInstruction* clone() override { return new VanadisOrImmInstruction(*this); }

    virtual VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }

    virtual const char* getInstCode() const override { return "ORI"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "ORI    %5" PRIu16 " <- %5" PRIu16 " | imm=%" PRIu64 " (phys: %5" PRIu16 " <- %5" PRIu16 " ^ %" PRIu64 ")",
            isa_int_regs_out[0], isa_int_regs_in[0], imm_value, phys_int_regs_out[0], phys_int_regs_in[0], imm_value);
    }

    virtual void log(SST::Output* output, int verboselevel, uint16_t sw_thr,
                            uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0,
                                    uint16_t phys_int_regs_in_1) override
    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) {
            output->verbose(
                CALL_INFO, verboselevel, 0,
                "hw_thr=%d sw_thr = %d Execute: 0x%" PRI_ADDR " %s phys: out=%" PRIu16 " in=%" PRIu16 " imm=%" PRIu64 ", isa: out=%" PRIu16
                " / in=%" PRIu16 "\n",
                getHWThread(),sw_thr,getInstructionAddress(),getInstCode(), phys_int_regs_out_0, phys_int_regs_in_0, imm_value, isa_int_regs_out[0],
                isa_int_regs_in[0]);
        }
        #endif
    }

    virtual void instOp(VanadisRegisterFile* regFile,
                                uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0) override
    {

        const uint64_t src_1 = regFile->getIntReg<uint64_t>(phys_int_regs_in_0);
        regFile->setIntReg<uint64_t>(phys_int_regs_out_0, (src_1) | imm_value);
    }

    virtual void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {

        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);


        instOp(regFile,phys_int_regs_out_0, phys_int_regs_in_0);
        #ifdef VANADIS_BUILD_DEBUG
        log(output, 16, 65535,phys_int_regs_out_0,phys_int_regs_in_0,0);
        #endif
        markExecuted();
    }

private:
    uint64_t imm_value;
};


} // namespace Vanadis
} // namespace SST

#endif
