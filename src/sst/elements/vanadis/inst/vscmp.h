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

#ifndef _H_VANADIS_SET_REG_COMPARE
#define _H_VANADIS_SET_REG_COMPARE

#include "inst/vcmptype.h"
#include "inst/vinst.h"

#include "util/vcmpop.h"

namespace SST {
namespace Vanadis {

class VanadisSetRegCompareInstruction : public VanadisInstruction {
public:
    VanadisSetRegCompareInstruction(const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
                                    const uint16_t dest, const uint16_t src_1, const uint16_t src_2, const bool sgnd,
                                    const VanadisRegisterCompareType cType, const VanadisRegisterFormat fmt)
        : VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0), performSigned(sgnd), compareType(cType),
          reg_format(fmt) {

        isa_int_regs_in[0] = src_1;
        isa_int_regs_in[1] = src_2;
        isa_int_regs_out[0] = dest;
    }

    VanadisSetRegCompareInstruction* clone() { return new VanadisSetRegCompareInstruction(*this); }

    virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_INT_ARITH; }
    virtual const char* getInstCode() const { return "CMPSET"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size) {
        snprintf(buffer, buffer_size,
                 "CMPSET (op: %s, %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16 " / phys-out: %" PRIu16
                 " phys-in: %" PRIu16 ", %" PRIu16 "\n",
                 convertCompareTypeToString(compareType), performSigned ? "signed" : "unsigned", isa_int_regs_out[0],
                 isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_in[0],
                 phys_int_regs_in[1]);
    }

    virtual void execute(SST::Output* output, VanadisRegisterFile* regFile) {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0,
                        "Execute: (addr=0x%0llx) CMPSET (op: %s, %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16
                        " / phys-out: %" PRIu16 " phys-in: %" PRIu16 ", %" PRIu16 "\n",
                        getInstructionAddress(), convertCompareTypeToString(compareType),
                        performSigned ? "signed" : "unsigned", isa_int_regs_out[0], isa_int_regs_in[0],
                        isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1]);
#endif
        bool compare_result = false;

        if (performSigned) {
            switch (reg_format) {
            case VANADIS_FORMAT_INT64: {
                compare_result = registerCompare<int64_t>(compareType, regFile, this, output, phys_int_regs_in[0],
                                                          phys_int_regs_in[1]);
            } break;
            case VANADIS_FORMAT_INT32: {
                compare_result = registerCompare<int32_t>(compareType, regFile, this, output, phys_int_regs_in[0],
                                                          phys_int_regs_in[1]);
            } break;
            default: {
                flagError();
            } break;
            }
        } else {
            switch (reg_format) {
            case VANADIS_FORMAT_INT64: {
                compare_result = registerCompare<uint64_t>(compareType, regFile, this, output, phys_int_regs_in[0],
                                                           phys_int_regs_in[1]);
            } break;
            case VANADIS_FORMAT_INT32: {
                compare_result = registerCompare<uint32_t>(compareType, regFile, this, output, phys_int_regs_in[0],
                                                           phys_int_regs_in[1]);
            } break;
            default: {
                flagError();
            } break;
            }
        }

        const uint16_t result_reg = phys_int_regs_out[0];

        if (compare_result) {
            const uint64_t vanadis_one = 1;
            regFile->setIntReg(result_reg, vanadis_one);
        } else {
            const uint64_t vanadis_zero = 0;
            regFile->setIntReg(result_reg, vanadis_zero);
        }

        markExecuted();
    }

protected:
    const bool performSigned;
    const VanadisRegisterCompareType compareType;
    const VanadisRegisterFormat reg_format;
};

} // namespace Vanadis
} // namespace SST

#endif
