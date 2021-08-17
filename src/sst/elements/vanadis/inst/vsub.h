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

#ifndef _H_VANADIS_SUB
#define _H_VANADIS_SUB

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template<VanadisRegisterFormat register_format>
class VanadisSubInstruction : public VanadisInstruction {
public:
    VanadisSubInstruction(const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
                          const uint16_t dest, const uint16_t src_1, const uint16_t src_2, bool trapOverflw)
        : VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0), trapOverflow(trapOverflw) {

        isa_int_regs_in[0] = src_1;
        isa_int_regs_in[1] = src_2;
        isa_int_regs_out[0] = dest;
    }

    VanadisSubInstruction* clone() override { return new VanadisSubInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char* getInstCode() const override { return "SUB"; }

    void printToBuffer(char* buffer, size_t buffer_size) override {
        snprintf(buffer, buffer_size,
                 "SUB   %5" PRIu16 " <- %5" PRIu16 " - %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " - %5" PRIu16
                 ")",
                 isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_in[0],
                 phys_int_regs_in[1]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0,
                        "Execute: (addr=%p) SUB phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16
                        " / in=%" PRIu16 ", %" PRIu16 "\n",
                        (void*)getInstructionAddress(), phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1],
                        isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1]);
#endif
        switch (register_format) {
        case VanadisRegisterFormat::VANADIS_FORMAT_INT64: {
            const int64_t src_1 = regFile->getIntReg<int64_t>(phys_int_regs_in[0]);
            const int64_t src_2 = regFile->getIntReg<int64_t>(phys_int_regs_in[1]);

            regFile->setIntReg<int64_t>(phys_int_regs_out[0], ((src_1) - (src_2)));
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_INT32: {
            const int32_t src_1 = regFile->getIntReg<int32_t>(phys_int_regs_in[0]);
            const int32_t src_2 = regFile->getIntReg<int32_t>(phys_int_regs_in[1]);

            regFile->setIntReg<int32_t>(phys_int_regs_out[0], ((src_1) - (src_2)));
        } break;
        default: {
            flagError();
        } break;
        }

        markExecuted();
    }

protected:
    const bool trapOverflow;
};

} // namespace Vanadis
} // namespace SST

#endif
