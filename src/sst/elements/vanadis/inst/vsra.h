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

#ifndef _H_VANADIS_SRA
#define _H_VANADIS_SRA

#include "inst/vinst.h"
#include "inst/vregfmt.h"

namespace SST {
namespace Vanadis {

class VanadisShiftRightArithmeticInstruction : public VanadisInstruction {
public:
    VanadisShiftRightArithmeticInstruction(const uint64_t addr, const uint32_t hw_thr,
                                           const VanadisDecoderOptions* isa_opts, const uint16_t dest,
                                           const uint16_t src_1, const uint16_t src_2, VanadisRegisterFormat fmt)
        : VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0), reg_format(fmt) {

        isa_int_regs_in[0] = src_1;
        isa_int_regs_in[1] = src_2;
        isa_int_regs_out[0] = dest;
    }

    virtual VanadisShiftRightArithmeticInstruction* clone() {
        return new VanadisShiftRightArithmeticInstruction(*this);
    }

    virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_INT_ARITH; }

    virtual const char* getInstCode() const { return "SRA"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size) {
        snprintf(buffer, buffer_size,
                 "SRA     %5" PRIu16 " <- %5" PRIu16 " >> %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " >> %5" PRIu16
                 ")",
                 isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_in[0],
                 phys_int_regs_in[1]);
    }

    virtual void execute(SST::Output* output, VanadisRegisterFile* regFile) {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0,
                        "Execute: (addr=%p) SRA phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16
                        " / in=%" PRIu16 ", %" PRIu16 "\n",
                        (void*)getInstructionAddress(), phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1],
                        isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1]);
#endif
        switch (reg_format) {
        case VANADIS_FORMAT_INT64: {
            const int64_t src_1 = regFile->getIntReg<int64_t>(phys_int_regs_in[0]);
            const int64_t src_2 = regFile->getIntReg<int64_t>(phys_int_regs_in[1]);

            if (0 == src_2) {
                const int32_t src_1_32 = regFile->getIntReg<int32_t>(phys_int_regs_in[0]);
                regFile->setIntReg<int32_t>(phys_int_regs_out[0], src_1_32);
            } else {
                regFile->setIntReg<int64_t>(phys_int_regs_out[0], src_1 >> src_2);
            }
        } break;
        case VANADIS_FORMAT_INT32: {
            const int32_t src_1 = regFile->getIntReg<int32_t>(phys_int_regs_in[0]);
            const int32_t src_2 = (int32_t)(regFile->getIntReg<uint32_t>(phys_int_regs_in[1]) & 0x1F);
            //                                const int32_t src_2 =
            //                                regFile->getIntReg<int32_t>(
            //                                phys_int_regs_in[1] );

            if (0 == src_2) {
                regFile->setIntReg<int32_t>(phys_int_regs_out[0], src_1);
            } else {
                regFile->setIntReg<int32_t>(phys_int_regs_out[0], src_1 >> src_2);
            }
        } break;
        case VANADIS_FORMAT_FP32:
        case VANADIS_FORMAT_FP64: {
            flagError();
        } break;
        }

        markExecuted();
    }

protected:
    VanadisRegisterFormat reg_format;
};

} // namespace Vanadis
} // namespace SST

#endif
