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

#ifndef _H_VANADIS_SETREG
#define _H_VANADIS_SETREG

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template<VanadisRegisterFormat register_format>
class VanadisSetRegisterInstruction : public VanadisInstruction {
public:
    VanadisSetRegisterInstruction(const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
                                  const uint16_t dest, const int64_t immediate)
        : VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0) {

        isa_int_regs_out[0] = dest;
        imm_value = immediate;
    }

    VanadisSetRegisterInstruction* clone() override { return new VanadisSetRegisterInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char* getInstCode() const override { return "SETREG"; }

    void printToBuffer(char* buffer, size_t buffer_size) override {
        snprintf(buffer, buffer_size, "SETREG  %5" PRIu16 " <- imm=%" PRId64 " (phys: %5" PRIu16 " <- %" PRId64 ")",
                 isa_int_regs_out[0], imm_value, phys_int_regs_out[0], imm_value);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0,
                        "Execute: (addr=0x%0llx) SETREG phys: out=%" PRIu16 " imm=%" PRId64 ", isa: out=%" PRIu16 "\n",
                        getInstructionAddress(), phys_int_regs_out[0], imm_value, isa_int_regs_out[0]);
#endif
        switch (register_format) {
        case VanadisRegisterFormat::VANADIS_FORMAT_INT64: {
            regFile->setIntReg<int64_t>(phys_int_regs_out[0], imm_value);
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_INT32: {
            regFile->setIntReg<int32_t>(phys_int_regs_out[0], static_cast<int32_t>(imm_value));
        } break;
        default: {
            flagError();
        } break;
        }
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0, "Result-reg %" PRIu16 ": %" PRId64 "\n", phys_int_regs_out[0], imm_value);
#endif
        markExecuted();
    }

private:
    int64_t imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
