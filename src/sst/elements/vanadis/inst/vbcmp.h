// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_BRANCH_REG_COMPARE
#define _H_VANADIS_BRANCH_REG_COMPARE

#include "inst/vcmptype.h"
#include "inst/vspeculate.h"
#include "util/vcmpop.h"

namespace SST {
namespace Vanadis {

template <typename register_format, VanadisRegisterCompareType compare_type>
class VanadisBranchRegCompareInstruction : public VanadisSpeculatedInstruction
{
public:
    VanadisBranchRegCompareInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint64_t ins_width,
        const uint16_t src_1, const uint16_t src_2, const int64_t offst, const VanadisDelaySlotRequirement delayT) :
        VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, ins_width, 2, 0, 2, 0, 0, 0, 0, 0, delayT),
        offset(offst)
    {
        isa_int_regs_in[0] = src_1;
        isa_int_regs_in[1] = src_2;
    }

    VanadisBranchRegCompareInstruction* clone() override { return new VanadisBranchRegCompareInstruction(*this); }
    const char*                         getInstCode() const override
    {
        switch ( compare_type ) {
        case REG_COMPARE_EQ:
            return "BCMP_EQ";
        case REG_COMPARE_NEQ:
            return "BCMP_NEQ";
        case REG_COMPARE_LT:
            return "BCMP_LT";
        case REG_COMPARE_LTE:
            return "BCMP_LTE";
        case REG_COMPARE_GT:
            return "BCMP_GT";
        case REG_COMPARE_GTE:
            return "BCMP_GTE";
        default:
            return "Unknown";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "BCMP (%s) isa-in: %" PRIu16 ", %" PRIu16 " / phys-in: %" PRIu16 ", %" PRIu16 " offset: %" PRId64
            " = 0x%" PRI_ADDR "",
            convertCompareTypeToString(compare_type), isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_in[0],
            phys_int_regs_in[1], offset, getInstructionAddress() + offset);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= 16) {
            std::ostringstream ss;
            ss << "Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
            ss << "(" << convertCompareTypeToString(compare_type) << ")"; 
            ss << " isa-in: " << isa_int_regs_in[0] << ", " << isa_int_regs_in[1];
            ss << " / phys-in: " << phys_int_regs_in[0] << ", " << phys_int_regs_in[1];
            ss << " offset: " <<  offset << " = " << getInstructionAddress() + offset ;
            output->verbose( CALL_INFO, 16, 0, "%s\n", ss.str().c_str());
        }
#endif
        bool compare_result = false;

        compare_result = registerCompare<compare_type, register_format>(
            regFile, this, output, phys_int_regs_in[0], phys_int_regs_in[1]);

        if ( compare_result ) {
            takenAddress = (uint64_t)(((int64_t)getInstructionAddress()) + offset);
#ifdef VANADIS_BUILD_DEBUG
            output->verbose(
                CALL_INFO, 16, 0, "-----> taken-address: 0x%" PRI_ADDR " + %" PRId64 " = 0x%" PRI_ADDR "\n", getInstructionAddress(),
                offset, takenAddress);
#endif
        }
        else {
            takenAddress = calculateStandardNotTakenAddress();
#ifdef VANADIS_BUILD_DEBUG
            output->verbose(CALL_INFO, 16, 0, "-----> not-taken-address: 0x%" PRI_ADDR "\n", takenAddress);
#endif
        }

        markExecuted();
    }

protected:
    const int64_t offset;
};

} // namespace Vanadis
} // namespace SST

#endif
