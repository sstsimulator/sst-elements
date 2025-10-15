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

#ifndef _H_VANADIS_JUMP_REG_LINK
#define _H_VANADIS_JUMP_REG_LINK

#include "inst/vinst.h"
#include "inst/vspeculate.h"

#include <cstdint>
#include <cstdio>

namespace SST {
namespace Vanadis {

class VanadisJumpRegLinkInstruction : public virtual VanadisSpeculatedInstruction
{

public:
    VanadisJumpRegLinkInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint64_t ins_width,
        const uint16_t returnAddrReg, const uint16_t jumpToAddrReg, const int64_t imm_jump,
        const VanadisDelaySlotRequirement delayT) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
        VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, ins_width, 1, 1, 1, 1, 0, 0, 0, 0, delayT)
    {

        isa_int_regs_in[0]  = jumpToAddrReg;
        isa_int_regs_out[0] = returnAddrReg;
        imm = imm_jump;
    }

    VanadisJumpRegLinkInstruction* clone() { return new VanadisJumpRegLinkInstruction(*this); }

    virtual const char* getInstCode() const { return "JLR"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size)
    {
        snprintf(
            buffer, buffer_size, "JLR     link-reg: %" PRIu16 " addr-reg: %" PRIu16 " + %" PRIu64 "",
            isa_int_regs_out[0], isa_int_regs_in[0], imm);
    }

    void log(SST::Output* output,int verboselevel, uint16_t sw_thr,
                    uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0,
                    uint64_t jump_to, uint64_t link_value)
    {
        #ifdef VANADIS_BUILD_DEBUG

        if(output->getVerboseLevel() >= verboselevel) {

            output->verbose(
                CALL_INFO, verboselevel, 0,
                "hw_thr=%d sw_thr = %d JLR isa-link: %" PRIu16 " isa-addr: %" PRIu16 " + %" PRIu64 " phys-link: %" PRIu16
                " phys-addr: %" PRIu16 " jump-to: 0x%0" PRI_ADDR " link-value: 0x%0" PRI_ADDR " takenAddr: 0x%0" PRI_ADDR "\n",
                 getHWThread(),sw_thr, isa_int_regs_out[0], isa_int_regs_in[0], imm, phys_int_regs_out_0,
                phys_int_regs_in_0,jump_to, link_value,takenAddress);
        }
        #endif
    }

    void instOp(VanadisRegisterFile* regFile,
        uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0,
        uint64_t* jump_to,uint64_t* link_value)
        {
            *jump_to = static_cast<uint64_t>(regFile->getIntReg<int64_t>(phys_int_regs_in_0) + imm);
            *link_value = calculateStandardNotTakenAddress();

            regFile->setIntReg<uint64_t>(phys_int_regs_out_0, *link_value);
            regFile->setIntReg<uint64_t>(phys_int_regs_in_0, *jump_to);
            takenAddress = regFile->getIntReg<uint64_t>(phys_int_regs_in_0);

        }

    virtual void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile)
    {

        uint16_t phys_int_regs_in_0 = phys_int_regs_in[0];
        uint16_t phys_int_regs_out_0 = phys_int_regs_out[0];
        uint64_t link_value = 0;
        uint64_t jump_to = 0;
        instOp(regFile, phys_int_regs_out_0, phys_int_regs_in_0, &jump_to, &link_value);
        log(output,16, 65535,phys_int_regs_out_0,phys_int_regs_in_0, jump_to, link_value);
        markExecuted();
    }

protected:
    int64_t imm;
};

} // namespace Vanadis
} // namespace SST

#endif
