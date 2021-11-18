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

#ifndef _H_VANADIS_JUMP_LINK
#define _H_VANADIS_JUMP_LINK

#include <cstdio>

#include "inst/vinst.h"
#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisJumpLinkInstruction : public VanadisSpeculatedInstruction {

public:
    VanadisJumpLinkInstruction(const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
										 const uint64_t ins_width, 
                               const uint16_t link_reg, const uint64_t pc, const VanadisDelaySlotRequirement delayT)
        : VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, ins_width, 0, 1, 0, 1, 0, 0, 0, 0, delayT) {

        isa_int_regs_out[0] = link_reg;
        takenAddress = pc;
    }

    VanadisJumpLinkInstruction* clone() override { return new VanadisJumpLinkInstruction(*this); }

    const char* getInstCode() const override { return "JL"; }

    void printToBuffer(char* buffer, size_t buffer_size) override {
        snprintf(buffer, buffer_size, "JL      %" PRIu64 " (0x%llx)", takenAddress, takenAddress);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override {
        const uint64_t link_value = calculateStandardNotTakenAddress();

#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0,
                        "Execute: JL jump-to: %" PRIu64 " / 0x%llx / link: %" PRIu16 " phys: %" PRIu16 " v: %" PRIu64
                        "/ 0x%llx\n",
                        takenAddress, takenAddress, isa_int_regs_out[0], phys_int_regs_out[0], link_value, link_value);
#endif
        regFile->setIntReg<uint64_t>(phys_int_regs_out[0], link_value);

//        if ((takenAddress & 0x3) != 0) {
//            flagError();
//        }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
