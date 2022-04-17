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

#ifndef _H_VANADIS_SLLI
#define _H_VANADIS_SLLI

#include "inst/vinst.h"
#include "inst/vregfmt.h"

namespace SST {
namespace Vanadis {

template <VanadisRegisterFormat register_format>
class VanadisShiftLeftLogicalImmInstruction : public VanadisInstruction
{
public:
    VanadisShiftLeftLogicalImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint64_t immediate) :
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
        if ( register_format == VanadisRegisterFormat::VANADIS_FORMAT_INT64 ) { return "SLLI64"; }
        else if ( register_format == VanadisRegisterFormat::VANADIS_FORMAT_INT32 ) {
            return "SLLI32";
        }
        else {
            return "SLLI";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%6s  %5" PRIu16 " <- %5" PRIu16 " << imm=%" PRId64 " (phys: %5" PRIu16 " <- %5" PRIu16 " << %" PRId64 ")",
            getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0], imm_value, phys_int_regs_out[0],
            phys_int_regs_in[0], imm_value);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: (addr=%p) %s phys: out=%" PRIu16 " in=%" PRIu16 " imm=%" PRId64 ", isa: out=%" PRIu16
            " / in=%" PRIu16 "\n",
            (void*)getInstructionAddress(), getInstCode(), phys_int_regs_out[0], phys_int_regs_in[0], imm_value,
            isa_int_regs_out[0], isa_int_regs_in[0]);
#endif
        assert(imm_value > 0);

        switch ( register_format ) {
        case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
        {
            const uint64_t src_1 = regFile->getIntReg<uint64_t>(phys_int_regs_in[0]);
            regFile->setIntReg<uint64_t>(phys_int_regs_out[0], src_1 << imm_value);
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
        {
            const uint32_t src_1 = regFile->getIntReg<uint32_t>(phys_int_regs_in[0]);
            regFile->setIntReg<uint32_t>(phys_int_regs_out[0], src_1 << static_cast<uint32_t>(imm_value));
        } break;
        default:
        {
            flagError();
        }
        }

        markExecuted();
    }

private:
    uint64_t imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
