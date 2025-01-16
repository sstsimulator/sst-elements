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

#ifndef _H_VANADIS_BRANCH_REG_COMPARE_IMM
#define _H_VANADIS_BRANCH_REG_COMPARE_IMM

#include "inst/vcmptype.h"
#include "inst/vspeculate.h"
#include "util/vcmpop.h"

namespace SST {
namespace Vanadis {

template <typename gpr_format, VanadisRegisterCompareType compareType>
class VanadisBranchRegCompareImmInstruction : public virtual VanadisSpeculatedInstruction
{
public:
    VanadisBranchRegCompareImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint64_t ins_width,
        const uint16_t src_1, const gpr_format imm, const int64_t offst, const VanadisDelaySlotRequirement delayT) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 0, 1, 0, 0, 0, 0, 0),
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
        std::ostringstream ss;
        ss << getInstCode();
        ss << " isa-in: " << isa_int_regs_in[0] << " / phys-in: " <<  phys_int_regs_in[0];
        ss << " / imm: " <<  imm_value << " / offset: " << offset;
        ss << " = " << static_cast<int64_t>(getInstructionAddress()) + offset;
        strncpy( buffer, ss.str().c_str(), buffer_size );
    }

    void log(SST::Output* output, int verboselevel, uint32_t sw_thr,  bool compare_result, 
                            uint16_t phys_int_regs_in_0)
    {
         #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) {
            std::ostringstream ss;
            ss << "hw_thr="<<getHWThread()<<" sw_thr="<<sw_thr<< " Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
            ss << " isa-in: " <<  isa_int_regs_in[0] << " / phys-in: " << phys_int_regs_in_0;
            ss << " / imm: " << imm_value << " / offset: " << offset << " = " << static_cast<int64_t>(getInstructionAddress()) + offset; 
            if(compare_result)
            ss << "-----> taken-address: 0x"<<std::hex << takenAddress;
            else
            ss << "-----> not-taken-address: 0x"<<std::hex << takenAddress;
            output->verbose( CALL_INFO, verboselevel, 0, "%s\n", ss.str().c_str());
        }
        #endif
    }

     void instOp(SST::Output* output,VanadisRegisterFile* regFile, uint16_t phys_int_regs_in_0,
                 bool * compare_result)
    {
        *compare_result = registerCompareImm<compareType, gpr_format>(regFile, this, output, phys_int_regs_in_0, imm_value);

        if ( *compare_result ) {
            const int64_t instruction_address = getInstructionAddress();
            const int64_t ins_addr_and_offset = instruction_address + offset;

            takenAddress = static_cast<uint64_t>(ins_addr_and_offset);
            
        }
        else {
            takenAddress = calculateStandardNotTakenAddress();
        }
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);
        bool compare_result = false;
        instOp(output, regFile, phys_int_regs_in_0, &compare_result);
        log(output, 16, 65535,compare_result,phys_int_regs_in_0);
        markExecuted();
    }

protected:
    const int64_t offset;
    const gpr_format imm_value;
};




} // namespace Vanadis
} // namespace SST

#endif

