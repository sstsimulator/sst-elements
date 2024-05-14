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

#ifndef _H_VANADIS_ADDI_UNSIGNED
#define _H_VANADIS_ADDI_UNSIGNED

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template <typename register_format>
class VanadisAddImmUnsignedInstruction : public VanadisInstruction
{
public:
    VanadisAddImmUnsignedInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const register_format immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
        imm_value(immediate)
    {
        isa_int_regs_in[0]  = src_1;
        isa_int_regs_out[0] = dest;
    }

    VanadisAddImmUnsignedInstruction* clone() override { return new VanadisAddImmUnsignedInstruction(*this); }
    VanadisFunctionalUnitType         getInstFuncType() const override { return INST_INT_ARITH; }

    const char* getInstCode() const override {
        if(sizeof(register_format) == 4) {
            return "ADDIU32";
        } else if( sizeof(register_format) == 8) {
            return "ADDIU64"; 
        } else {
            return "ADDIUERR"; 
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "ADDIU   %5" PRIu16 " <- %5" PRIu16 " + imm=%" PRId64 " (phys: %5" PRIu16 " <- %5" PRIu16 " + %" PRId64 ")",
            isa_int_regs_out[0], isa_int_regs_in[0], imm_value, phys_int_regs_out[0], phys_int_regs_in[0], imm_value);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= 16) {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: (addr=%p) ADDIU phys: out=%" PRIu16 " in=%" PRIu16 " imm=%" PRId64 ", isa: out=%" PRIu16
                " / in=%" PRIu16 "\n",
                (void*)getInstructionAddress(), phys_int_regs_out[0], phys_int_regs_in[0], imm_value, isa_int_regs_out[0],
                isa_int_regs_in[0]);
        }
#endif

        if( std::is_same<register_format, uint64_t>::value || std::is_same<register_format, uint32_t>::value ) {
            const register_format src_1 = regFile->getIntReg<register_format>(phys_int_regs_in[0]);
            regFile->setIntReg<register_format>(phys_int_regs_out[0], src_1 + imm_value, false);
        } else {
            flagError();
        }

        markExecuted();
    }

private:
    const register_format imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
