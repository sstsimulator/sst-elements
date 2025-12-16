// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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
class VanadisBranchRegCompareInstruction : public virtual VanadisSpeculatedInstruction
{
public:
    VanadisBranchRegCompareInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint64_t ins_width,
        const uint16_t src_1, const uint16_t src_2, const int64_t offst, const VanadisDelaySlotRequirement delayT) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 0, 2, 0, 0, 0, 0, 0),
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

    void log(SST::Output* output, int verboselevel, uint32_t sw_thr,  bool compare_result,
                            uint16_t phys_int_regs_in_0,uint16_t phys_int_regs_in_1)

    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) {
            std::ostringstream ss;
            ss << "hw_thr="<<getHWThread()<<" sw_thr="<<sw_thr<<" Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
            ss << "(" << convertCompareTypeToString(compare_type) << ")";
            ss << " isa-in: " << isa_int_regs_in[0] << ", " << isa_int_regs_in[1];
            ss << " / phys-in: " << phys_int_regs_in_0 << ", " << phys_int_regs_in_1;
            ss << " offset: " <<  offset << " = " << getInstructionAddress() + offset ;
            if(compare_result)
            ss << "-----> taken-address: 0x"<<std::hex << takenAddress;
            else
            ss << "-----> not-taken-address: 0x"<<std::hex << takenAddress;
            output->verbose( CALL_INFO, verboselevel, 0, "%s\n", ss.str().c_str());
        }
        #endif
    }

    void instOp(SST::Output* output,VanadisRegisterFile* regFile, uint16_t phys_int_regs_in_0,
                uint16_t phys_int_regs_in_1, bool * compare_result)
    {
        *compare_result = registerCompare<compare_type, register_format>(
            regFile, this, output, phys_int_regs_in_0, phys_int_regs_in_1);

        if ( *compare_result ) {
            takenAddress = (uint64_t)(((int64_t)getInstructionAddress()) + offset);

        }
        else {
            takenAddress = calculateStandardNotTakenAddress();
        }
    }
    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_in_0 = phys_int_regs_in[0];
        uint16_t phys_int_regs_in_1 = phys_int_regs_in[1];
        bool compare_result = false;
        instOp(output, regFile, phys_int_regs_in_0, phys_int_regs_in_1,&compare_result);
        #ifdef VANADIS_BUILD_DEBUG
        log(output, 16, 65535,compare_result,phys_int_regs_in_0,phys_int_regs_in_1);
        #endif
        markExecuted();
    }

protected:
    const int64_t offset;
};



} // namespace Vanadis
} // namespace SST

#endif
