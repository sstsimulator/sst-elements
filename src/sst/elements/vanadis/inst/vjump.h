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

#ifndef _H_VANADIS_JUMP
#define _H_VANADIS_JUMP

#include "inst/vinst.h"
#include "inst/vspeculate.h"

#include <cstdio>

namespace SST {
namespace Vanadis {

class VanadisJumpInstruction : public VanadisSpeculatedInstruction
{

public:
    VanadisJumpInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint64_t ins_width,
        const uint64_t pc, const VanadisDelaySlotRequirement delayT) :
        VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, ins_width, 0, 0, 0, 0, 0, 0, 0, 0, delayT)
    {

        takenAddress = pc;
    }

    VanadisJumpInstruction* clone() override { return new VanadisJumpInstruction(*this); }

    const char* getInstCode() const override { return "JMP"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(buffer, buffer_size, "JUMP    %" PRIu64 " / 0x%llx", takenAddress, takenAddress);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override { markExecuted(); }
};

} // namespace Vanadis
} // namespace SST

#endif
