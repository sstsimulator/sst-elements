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

#ifndef _H_VANADIS_BRANCH_REG_COMPARE_IMM
#define _H_VANADIS_BRANCH_REG_COMPARE_IMM

#include "inst/vcmptype.h"
#include "inst/vspeculate.h"
#include "util/vcmpop.h"

namespace SST {
namespace Vanadis {

template <VanadisRegisterFormat register_format, VanadisRegisterCompareType compareType>
class VanadisBranchRegCompareImmInstruction : public VanadisSpeculatedInstruction
{
public:
    VanadisBranchRegCompareImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint64_t ins_width,
        const uint16_t src_1, const int64_t imm, const int64_t offst, const VanadisDelaySlotRequirement delayT) :
        VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, ins_width, 1, 0, 1, 0, 0, 0, 0, 0, delayT),
        imm_value(imm),
        offset(offst)
    {

        isa_int_regs_in[0] = src_1;
    }

    VanadisBranchRegCompareImmInstruction* clone() override { return new VanadisBranchRegCompareImmInstruction(*this); }
    const char*                            getInstCode() const override { return "BCMPI"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "BCMPI isa-in: %" PRIu16 " / phys-in: %" PRIu16 " / imm: %" PRId64 " / offset: %" PRId64 " = 0x%llx\n",
            isa_int_regs_in[0], phys_int_regs_in[0], imm_value, offset,
            static_cast<int64_t>(getInstructionAddress()) + offset);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: (addr=0x%0llx) BCMPI isa-in: %" PRIu16 " / phys-in: %" PRIu16 " / imm: %" PRId64
            " / offset: %" PRId64 " = (0x%llx) \n",
            getInstructionAddress(), isa_int_regs_in[0], phys_int_regs_in[0], imm_value, offset,
            static_cast<int64_t>(getInstructionAddress()) + offset);
#endif
        bool compare_result = false;

        switch ( register_format ) {
        case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
        {
            compare_result =
                registerCompareImm<compareType, int64_t>(regFile, this, output, phys_int_regs_in[0], imm_value);
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
        {
            compare_result = registerCompareImm<compareType, int32_t>(
                regFile, this, output, phys_int_regs_in[0], static_cast<int32_t>(imm_value));
        } break;
        default:
        {
            flagError();
        } break;
        }

        if ( compare_result ) {
            const int64_t instruction_address = getInstructionAddress();
            const int64_t ins_addr_and_offset = instruction_address + offset;

            takenAddress = static_cast<uint64_t>(ins_addr_and_offset);
        }
        else {
            takenAddress = calculateStandardNotTakenAddress();
        }

        markExecuted();
    }

protected:
    const int64_t offset;
    const int64_t imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
