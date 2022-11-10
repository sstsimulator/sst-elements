// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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

template <VanadisRegisterCompareType compare_type, VanadisRegisterFormat register_format, bool perform_signed>
class VanadisSetRegCompareInstruction : public VanadisInstruction
{
public:
    VanadisSetRegCompareInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0)
    {

        isa_int_regs_in[0]  = src_1;
        isa_int_regs_in[1]  = src_2;
        isa_int_regs_out[0] = dest;
    }

    VanadisSetRegCompareInstruction* clone() override { return new VanadisSetRegCompareInstruction(*this); }

    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char*               getInstCode() const override { return "CMPSET"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "CMPSET (op: %s, %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16 " / phys-out: %" PRIu16
            " phys-in: %" PRIu16 ", %" PRIu16 "\n",
            convertCompareTypeToString(compare_type), perform_signed ? "signed" : "unsigned", isa_int_regs_out[0],
            isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= 16) {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: (addr=0x%0llx) CMPSET (op: %s, %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16
                " / phys-out: %" PRIu16 " phys-in: %" PRIu16 ", %" PRIu16 "\n",
                getInstructionAddress(), convertCompareTypeToString(compare_type), perform_signed ? "signed" : "unsigned",
                isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_in[0],
                phys_int_regs_in[1]);
        }
#endif
        bool compare_result = false;

        if ( perform_signed ) {
            switch ( register_format ) {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
            {
                compare_result = registerCompare<compare_type, int64_t>(
                    regFile, this, output, phys_int_regs_in[0], phys_int_regs_in[1]);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
            {
                compare_result = registerCompare<compare_type, int32_t>(
                    regFile, this, output, phys_int_regs_in[0], phys_int_regs_in[1]);
            } break;
            default:
            {
                flagError();
            } break;
            }
        }
        else {
            switch ( register_format ) {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
            {
                compare_result = registerCompare<compare_type, uint64_t>(
                    regFile, this, output, phys_int_regs_in[0], phys_int_regs_in[1]);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
            {
                compare_result = registerCompare<compare_type, uint32_t>(
                    regFile, this, output, phys_int_regs_in[0], phys_int_regs_in[1]);
            } break;
            default:
            {
                flagError();
            } break;
            }
        }

        const uint16_t result_reg = phys_int_regs_out[0];

        if ( compare_result ) {
            const uint64_t vanadis_one = 1;
            regFile->setIntReg(result_reg, vanadis_one);
        }
        else {
            const uint64_t vanadis_zero = 0;
            regFile->setIntReg(result_reg, vanadis_zero);
        }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
