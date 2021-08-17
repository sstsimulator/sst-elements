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

#ifndef _H_VANADIS_DIV_REMAIN
#define _H_VANADIS_DIV_REMAIN

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template<VanadisRegisterFormat register_format, bool perform_signed>
class VanadisDivideRemainderInstruction : public VanadisInstruction {
public:
    VanadisDivideRemainderInstruction(const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
                                      const uint16_t quo_dest, const uint16_t remain_dest, const uint16_t src_1,
                                      const uint16_t src_2)
        : VanadisInstruction(addr, hw_thr, isa_opts, 2, 2, 2, 2, 0, 0, 0, 0) {

        isa_int_regs_in[0] = src_1;
        isa_int_regs_in[1] = src_2;
        isa_int_regs_out[0] = quo_dest;
        isa_int_regs_out[1] = remain_dest;
    }

    VanadisDivideRemainderInstruction* clone() override { return new VanadisDivideRemainderInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_DIV; }
    const char* getInstCode() const override { return "DIVREM"; }

    void printToBuffer(char* buffer, size_t buffer_size) override {
        snprintf(buffer, buffer_size,
                 "DIVREM%c q: %5" PRIu16 " r: %" PRIu16 " <- %" PRIu16 " \\ %" PRIu16 ", (phys: q: %" PRIu16
                 " r: %" PRIu16 " r: %" PRIu16 " %" PRIu16 " )\n",
                 perform_signed ? ' ' : 'U', isa_int_regs_out[0], isa_int_regs_out[1], isa_int_regs_in[0],
                 isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_out[1], phys_int_regs_in[0],
                 phys_int_regs_in[1]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0,
                        "Execute: (addr=%p) DIVREM%c q: %" PRIu16 " r: %" PRIu16 " <- %" PRIu16 " \\ %" PRIu16
                        " (phys: q: %" PRIu16 " r: %" PRIu16 " %" PRIu16 " %" PRIu16 ")\n",
                        (void*)getInstructionAddress(), perform_signed ? ' ' : 'U', isa_int_regs_out[0],
                        isa_int_regs_out[1], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0],
                        phys_int_regs_out[1], phys_int_regs_in[0], phys_int_regs_in[1]);
#endif
        if (perform_signed) {
            switch (register_format) {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64: {
                const int64_t src_1 = regFile->getIntReg<int64_t>(phys_int_regs_in[0]);
                const int64_t src_2 = regFile->getIntReg<int64_t>(phys_int_regs_in[1]);

                if (0 == src_2) {
                    flagError();
                } else {
                    const int64_t quo = (src_1) / (src_2);
                    const int64_t mod = (src_1) % (src_2);
#ifdef VANADIS_BUILD_DEBUG
                    output->verbose(CALL_INFO, 16, 0,
                                    "--> Execute: (detailed, signed, DIVREM64) %" PRId64 " / %" PRId64 " = (q: %" PRId64
                                    ", r: %" PRId64 ")\n",
                                    src_1, src_2, quo, mod);
#endif
                    regFile->setIntReg<int64_t>(phys_int_regs_out[0], quo, true);
                    regFile->setIntReg<int64_t>(phys_int_regs_out[1], mod, true);
                }
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32: {
                const int32_t src_1 = regFile->getIntReg<int32_t>(phys_int_regs_in[0]);
                const int32_t src_2 = regFile->getIntReg<int32_t>(phys_int_regs_in[1]);

                if (0 == src_2) {
                    flagError();
                } else {
                    const int32_t quo = (src_1) / (src_2);
                    const int32_t mod = (src_1) % (src_2);
#ifdef VANADIS_BUILD_DEBUG
                    output->verbose(CALL_INFO, 16, 0,
                                    "--> Execute: (detailed, signed, DIVREM32) %" PRId32 " / %" PRId32 " = (q: %" PRId32
                                    ", r: %" PRId32 ")\n",
                                    src_1, src_2, quo, mod);
#endif
                    regFile->setIntReg<int32_t>(phys_int_regs_out[0], quo, true);
                    regFile->setIntReg<int32_t>(phys_int_regs_out[1], mod, true);
                }
            } break;
            default: {
                flagError();
            } break;
            }
        } else {
            switch (register_format) {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64: {
                const uint64_t src_1 = regFile->getIntReg<uint64_t>(phys_int_regs_in[0]);
                const uint64_t src_2 = regFile->getIntReg<uint64_t>(phys_int_regs_in[1]);

                if (0 == src_2) {
                    // Behavior of a DIVU in MIPS is undefined
                    // flagError();
                } else {
                    const uint64_t quo = (src_1) / (src_2);
                    const uint64_t mod = (src_1) % (src_2);
#ifdef VANADIS_BUILD_DEBUG
                    output->verbose(CALL_INFO, 16, 0,
                                    "--> Execute: (detailed, unsigned, DIVREM64) %" PRIu64 " / %" PRIu64
                                    " = (q: %" PRIu64 ", r: %" PRIu64 ")\n",
                                    src_1, src_2, quo, mod);
#endif
                    regFile->setIntReg<uint64_t>(phys_int_regs_out[0], quo, false);
                    regFile->setIntReg<uint64_t>(phys_int_regs_out[1], mod, false);
                }
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32: {
                const uint32_t src_1 = regFile->getIntReg<uint32_t>(phys_int_regs_in[0]);
                const uint32_t src_2 = regFile->getIntReg<uint32_t>(phys_int_regs_in[1]);

                if (0 == src_2) {
                    // Behavior of a DIVU in MIPS is undefined
                    // flagError();
                } else {
                    const uint32_t quo = (src_1) / (src_2);
                    const uint32_t mod = (src_1) % (src_2);
#ifdef VANADIS_BUILD_DEBUG
                    output->verbose(CALL_INFO, 16, 0,
                                    "--> Execute: (detailed, unsigned, DIVREM32) %" PRIu32 " / %" PRIu32
                                    " = (q: %" PRIu32 ", r: %" PRIu32 ")\n",
                                    src_1, src_2, quo, mod);
#endif
                    regFile->setIntReg<uint32_t>(phys_int_regs_out[0], quo, false);
                    regFile->setIntReg<uint32_t>(phys_int_regs_out[1], mod, false);
                }
            } break;
            default: {
                flagError();
            } break;
            }
        }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
