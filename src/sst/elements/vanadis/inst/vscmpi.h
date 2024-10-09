// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
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
class VanadisSetRegCompareImmInstruction : public VanadisInstruction
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

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= 16) {
            std::ostringstream ss;

            ss << "Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
            ss << " (op: ";
            ss << convertCompareTypeToString(compare_type);
            ss << ", ";
            ss << (std::is_signed<register_format>::value ? "signed" : "unsigned"); 
            ss << ")";
            ss << " isa-out: "    << isa_int_regs_out[0]  << " isa-in: "  << isa_int_regs_in[0];
            ss << " / phys-out: " << phys_int_regs_out[0] << " phys-in: " << phys_int_regs_in[0];
            ss << " / imm: " << imm_value;
            output->verbose( CALL_INFO, 16, 0, "%s\n", ss.str().c_str());
        }
#endif
        const bool compare_result = registerCompareImm<compare_type, register_format>(regFile, 
                this, output, phys_int_regs_in[0], imm_value);

        if ( compare_result ) { 
            regFile->setIntReg<uint64_t>(phys_int_regs_out[0], static_cast<uint64_t>(1)); 
        } else {
            regFile->setIntReg<uint64_t>(phys_int_regs_out[0], static_cast<uint64_t>(0));
        }

        markExecuted();
    }

protected:
    const register_format imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
