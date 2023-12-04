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

#ifndef _H_VANADIS_SLLI
#define _H_VANADIS_SLLI

#include "inst/vinst.h"
#include "inst/vregfmt.h"

namespace SST {
namespace Vanadis {

template <typename register_format>
class VanadisShiftLeftLogicalImmInstruction : public VanadisInstruction
{
public:
    VanadisShiftLeftLogicalImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const register_format immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0)
    {
        isa_int_regs_in[0]  = src_1;
        isa_int_regs_out[0] = dest;

        imm_value = immediate;
    }

    VanadisShiftLeftLogicalImmInstruction* clone() override { return new VanadisShiftLeftLogicalImmInstruction(*this); }
    VanadisFunctionalUnitType              getInstFuncType() const override { return INST_INT_ARITH; }
    const char*                            getInstCode() const override
    {
        switch(sizeof(register_format)) {
        case 4:
            return "SLLI32";
        case 8:
            return "SLLI64";
        default:
            return "SLLI";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        std::ostringstream ss;
        ss << getInstCode();
        ss << " "       << isa_int_regs_out[0]  << " <- " << isa_int_regs_in[0]  << " << imm=" << imm_value;
        ss << " phys: " << phys_int_regs_out[0] << " <- " << phys_int_regs_in[0] << " << imm=" << imm_value;

        strncpy( buffer, ss.str().c_str(), buffer_size );
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= 16) {

            std::ostringstream ss;
            ss << "Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
            ss << " phys: out=" <<  phys_int_regs_out[0] << " in=" << phys_int_regs_in[0];
            ss << " imm=" << imm_value;
            ss << ", isa: out=" <<  isa_int_regs_out[0]  << " in=" << isa_int_regs_in[0];

            output->verbose( CALL_INFO, 16, 0, "%s\n", ss.str().c_str());
        }
#endif
        if constexpr ( sizeof( register_format ) == 4 ) {
            // imm cannot be 0 for RV32 or for RV64 when working on 32 bit values
            if ( UNLIKELY( 0 == imm_value ) ) {
                auto str = getenv("VANADIS_NO_FAULT");
                if ( nullptr == str ) {
                    flagError();
                }
            }
        }

        const register_format src_1 = regFile->getIntReg<register_format>(phys_int_regs_in[0]);
        regFile->setIntReg<register_format>(phys_int_regs_out[0], src_1 << imm_value);

        markExecuted();
    }

private:
    register_format imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
