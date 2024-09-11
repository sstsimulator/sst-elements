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

#ifndef _H_VANADIS_SET_REG_COMPARE_IMM
#define _H_VANADIS_SET_REG_COMPARE_IMM

#include "inst/vcmptype.h"
#include "inst/vinst.h"
#include "util/vcmpop.h"

namespace SST {
namespace Vanadis {

template <VanadisRegisterCompareType compare_type, typename register_format>
class VanadisSetRegCompareImmInstruction : public virtual VanadisInstruction
{
public:
    VanadisSetRegCompareImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const register_format imm) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
        imm_value(imm)
    {
        isa_int_regs_in[0]  = src_1;
        isa_int_regs_out[0] = dest;
    }

    VanadisSetRegCompareImmInstruction* clone() override { return new VanadisSetRegCompareImmInstruction(*this); }

    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char*               getInstCode() const override { return "CMPSETI"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        std::ostringstream ss;
        ss << getInstCode();
        ss << "(op: ";
        ss << convertCompareTypeToString(compare_type);
        ss << ", ";
        ss << (std::is_signed<register_format>::value ? "signed" : "unsigned");
        ss << ")";
        ss << " isa-out: "    <<  isa_int_regs_out[0] << " isa-in: "  <<  isa_int_regs_in[0];
        ss << " / phys-out: " << phys_int_regs_out[0] << " phys-in: " <<  phys_int_regs_in[0]; 
        ss << " / imm: " << imm_value; 
        strncpy( buffer, ss.str().c_str(), buffer_size );
    }

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr, 
        uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0,uint16_t phys_int_regs_in_1) override
        {
            #ifdef VANADIS_BUILD_DEBUG
            if(output->getVerboseLevel() >=  verboselevel) {
                std::ostringstream ss;

                ss << "hw_thr="<<getHWThread()<<" sw_thr=" <<sw_thr<<" Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
                ss << " (op: ";
                ss << convertCompareTypeToString(compare_type);
                ss << ", ";
                ss << (std::is_signed<register_format>::value ? "signed" : "unsigned"); 
                ss << ")";
                ss << " isa-out: "    << isa_int_regs_out[0]  << " isa-in: "  << isa_int_regs_in[0];
                ss << " / phys-out: " << phys_int_regs_out_0 << " phys-in: " << phys_int_regs_in_0;
                ss << " / imm: " << imm_value;
                output->verbose( CALL_INFO, verboselevel, 0, "%s\n", ss.str().c_str());
            }
            #endif
        }

    void instOp(SST::Output* output, VanadisRegisterFile* regFile, 
        uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0)
    {
        
        const bool compare_result = registerCompareImm<compare_type, register_format>(regFile, 
                this, output, phys_int_regs_in_0, imm_value);
        if ( compare_result ) { 
            regFile->setIntReg<uint64_t>(phys_int_regs_out_0, static_cast<uint64_t>(1)); 
        } else {
            regFile->setIntReg<uint64_t>(phys_int_regs_out_0, static_cast<uint64_t>(0));
        }
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        instOp(output, regFile, phys_int_regs_out_0, phys_int_regs_in_0);
        log(output, 16, 65535, phys_int_regs_out_0, phys_int_regs_in_0,0);
        markExecuted();
    }

protected:
    const register_format imm_value;
};

template <VanadisRegisterCompareType compare_type, typename register_format>
class VanadisSIMTSetRegCompareImmInstruction : public VanadisSIMTInstruction, public VanadisSetRegCompareImmInstruction<compare_type, register_format>
{
public:
    VanadisSIMTSetRegCompareImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const register_format imm) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
        VanadisSIMTInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
        VanadisSetRegCompareImmInstruction<compare_type, register_format>(addr, hw_thr, isa_opts, dest, src_1, imm)
    {
        ;
    }

    VanadisSIMTSetRegCompareImmInstruction* clone() override { return new VanadisSIMTSetRegCompareImmInstruction(*this); }
    
    void simtExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0,VanadisSIMTInstruction::sw_thread);
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0,VanadisSIMTInstruction::sw_thread);
        VanadisSetRegCompareImmInstruction<compare_type, register_format>::instOp(output, regFile, phys_int_regs_out_0, phys_int_regs_in_0);
        VanadisSetRegCompareImmInstruction<compare_type, register_format>::log(output, 16,VanadisSIMTInstruction::sw_thread, phys_int_regs_out_0, phys_int_regs_in_0,0);
    }

};

} // namespace Vanadis
} // namespace SST

#endif
