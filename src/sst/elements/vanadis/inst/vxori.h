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

#ifndef _H_VANADIS_XORI
#define _H_VANADIS_XORI

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisXorImmInstruction : public virtual VanadisInstruction
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
            " (0x%" PRI_ADDR "))",
            isa_int_regs_out[0], isa_int_regs_in[0], imm_value, phys_int_regs_out[0], phys_int_regs_in[0], imm_value,
            imm_value);
    }

    virtual void log(SST::Output* output, int verboselevel, uint16_t sw_thr,
                uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0) override
    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) {

            std::ostringstream ss;
            ss << "hw_thr="<<getHWThread()<<" sw_thr=" <<sw_thr;
            ss << " Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
            ss << " phys: out=" <<  phys_int_regs_out_0 << " in=" << phys_int_regs_in_0;
            ss << " imm=" << imm_value;
            ss << ", isa: out=" <<  isa_int_regs_out[0]  << " in=" << isa_int_regs_in[0];
            output->verbose( CALL_INFO, verboselevel, 0, "%s\n", ss.str().c_str());
        }
        #endif
    }

    void instOp(VanadisRegisterFile* regFile,
                                uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0) override
    {
        const uint64_t src_1 = regFile->getIntReg<uint64_t>(phys_int_regs_in_0);
        regFile->setIntReg<uint64_t>(phys_int_regs_out_0, (src_1) ^ imm_value);
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        instOp(regFile, phys_int_regs_out_0, phys_int_regs_in_0);
        #ifdef VANADIS_BUILD_DEBUG
        log(output, 16, 65535, phys_int_regs_out_0, phys_int_regs_in_0);
        #endif
        markExecuted();
    }

private:
    uint64_t imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
