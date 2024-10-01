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

#ifndef _H_VANADIS_JUMP_LINK
#define _H_VANADIS_JUMP_LINK

#include "inst/vinst.h"
#include "inst/vspeculate.h"

#include <cstdio>

namespace SST {
namespace Vanadis {

class VanadisJumpLinkInstruction : public virtual VanadisSpeculatedInstruction
{

public:
    VanadisJumpLinkInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint64_t ins_width,
        const uint16_t link_reg, const uint64_t pc, const VanadisDelaySlotRequirement delayT) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0),
        VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, ins_width, 0, 1, 0, 1, 0, 0, 0, 0, delayT)
    {

        isa_int_regs_out[0] = link_reg;
        takenAddress        = pc;
    }

    VanadisJumpLinkInstruction* clone() override { return new VanadisJumpLinkInstruction(*this); }

    const char* getInstCode() const override { return "JL"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(buffer, buffer_size, "JL      %" PRIu64 " (0x%" PRI_ADDR ")", takenAddress, takenAddress);
    }

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr, 
                uint64_t link_val, uint16_t phys_int_regs_out0, uint64_t takenAddr)
    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) 
        {
            output->verbose(
                CALL_INFO, verboselevel, 0,
                "hw_thr=%d sw_thr = %d JumpExecute: 0x%" PRI_ADDR " JL jump-to: %" PRIu64 " / 0x%" PRI_ADDR " / link: %" PRIu16 " phys: %" PRIu16 " v: %" PRIu64 "/ 0x%" PRI_ADDR "\n",
                getHWThread(),sw_thr, getInstructionAddress(),takenAddr, takenAddr,
                 isa_int_regs_out[0], phys_int_regs_out0, link_val, link_val);
        }
        #endif
    }

    void instOp(VanadisRegisterFile* regFile,uint16_t phys_int_regs_out0, uint64_t link_val)
    {
        regFile->setIntReg<uint64_t>(phys_int_regs_out0, link_val);
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        const uint64_t link_value = calculateStandardNotTakenAddress();
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        log(output, 16, 65355, link_value, phys_int_regs_out_0, takenAddress);
        instOp(regFile, phys_int_regs_out_0, link_value );
        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
