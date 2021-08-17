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

#include <cstdio>

#include "inst/vinst.h"
#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisJumpInstruction : public VanadisSpeculatedInstruction {

public:
    VanadisJumpInstruction(const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
                           const uint64_t pc, const VanadisDelaySlotRequirement delayT)
        : VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0, 0, 0, 0, 0, delayT) {

        takenAddress = pc;
    }

    virtual VanadisJumpInstruction* clone() { return new VanadisJumpInstruction(*this); }

    virtual const char* getInstCode() const { return "JMP"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size) {
        snprintf(buffer, buffer_size, "JUMP    %" PRIu64 "", takenAddress);
    }

    virtual void execute(SST::Output* output, VanadisRegisterFile* regFile) {
        if ((takenAddress & 0x3) != 0) {
            flagError();
        }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
