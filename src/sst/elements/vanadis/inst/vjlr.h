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

#ifndef _H_VANADIS_JUMP_REG_LINK
#define _H_VANADIS_JUMP_REG_LINK

#include <cstdio>

#include "inst/vinst.h"
#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisJumpRegLinkInstruction : public VanadisSpeculatedInstruction {

public:
    VanadisJumpRegLinkInstruction(const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
											 const uint64_t ins_width,
                                  const uint16_t returnAddrReg, const uint16_t jumpToAddrReg,
											 const int64_t imm_jump,
                                  const VanadisDelaySlotRequirement delayT)
        : VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, ins_width, 1, 1, 1, 1, 0, 0, 0, 0, delayT), imm(imm_jump) {

        isa_int_regs_in[0] = jumpToAddrReg;
        isa_int_regs_out[0] = returnAddrReg;
    }

    VanadisJumpRegLinkInstruction* clone() { return new VanadisJumpRegLinkInstruction(*this); }

    virtual const char* getInstCode() const { return "JLR"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size) {
        snprintf(buffer, buffer_size, "JLR     link-reg: %" PRIu16 " addr-reg: %" PRIu16 " + %" PRIu64 "\n", isa_int_regs_out[0],
                 isa_int_regs_in[0], imm);
    }

    virtual void execute(SST::Output* output, VanadisRegisterFile* regFile) {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0,
                        "Execute: addr=(0x%0llx) JLR isa-link: %" PRIu16 " isa-addr: %" PRIu16 " + %" PRIu64 " phys-link: %" PRIu16
                        " phys-addr: %" PRIu16 "\n",
                        getInstructionAddress(), isa_int_regs_out[0], isa_int_regs_in[0], imm, phys_int_regs_out[0],
                        phys_int_regs_in[0]);
#endif
        const uint64_t jump_to = static_cast<uint64_t>(regFile->getIntReg<int64_t>(phys_int_regs_in[0]) + imm);
        const uint64_t link_value = calculateStandardNotTakenAddress();

        regFile->setIntReg<uint64_t>(phys_int_regs_out[0], link_value);

#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0, "Execute JLR jump-to: 0x%0llx link-value: 0x%0llx\n", jump_to, link_value);
#endif
        takenAddress = regFile->getIntReg<uint64_t>(phys_int_regs_in[0]);

		  // TODO remove this code and check?
//        if ((takenAddress & 0x3) != 0) {
//            flagError();
//        }

        markExecuted();
    }

protected:

int64_t imm;

};

} // namespace Vanadis
} // namespace SST

#endif
