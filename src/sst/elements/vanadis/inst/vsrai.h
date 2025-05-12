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

#ifndef _H_VANADIS_SRAI
#define _H_VANADIS_SRAI

#include "inst/vinst.h"
#include "inst/vregfmt.h"

namespace SST {
namespace Vanadis {

template <VanadisRegisterFormat register_format>
class VanadisShiftRightArithmeticImmInstruction : public virtual VanadisInstruction
{
public:
    VanadisShiftRightArithmeticImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const int64_t immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0)
    {

        isa_int_regs_in[0]  = src_1;
        isa_int_regs_out[0] = dest;

        imm_value = immediate;
    }

    VanadisShiftRightArithmeticImmInstruction* clone() override
    {
        return new VanadisShiftRightArithmeticImmInstruction(*this);
    }

    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char*               getInstCode() const override { return "SRAI"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "SRAI    %5" PRIu16 " <- %5" PRIu16 " >> imm=%" PRId64 " (phys: %5" PRIu16 " <- %5" PRIu16 " >> %" PRId64
            ")",
            isa_int_regs_out[0], isa_int_regs_in[0], imm_value, phys_int_regs_out[0], phys_int_regs_in[0], imm_value);
    }

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr,
                uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0) override
    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) {

            std::ostringstream ss;
            ss << "hw_thr="<<getHWThread()<<" sw_thr=" <<sw_thr;
            ss << " Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
            ss << " phys: out=" <<  phys_int_regs_out_0 << " in=" << phys_int_regs_in_0;
            ss << " imm=" << imm_value;
            ss << ", isa: out=" <<  isa_int_regs_out[0]  << " in=" << isa_int_regs_in[0];
            output->verbose( CALL_INFO, verboselevel, 0, "%s\n", ss.str().c_str());
        }
        #endif
    }

    void instOp(VanadisRegisterFile* regFile,
                                uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0) override
    {
        if constexpr ( sizeof( register_format ) == 4 ) {
            // imm cannot be 0 for RV32 or for RV64 when working on 32 bit values
            if ( UNLIKELY( 0 == imm_value ) ) {
                auto str = getenv("VANADIS_NO_FAULT");
                if ( nullptr == str ) {
                    flagError();
                }
            }
        }

        switch ( register_format ) {
        case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
        {
            const int64_t src_1 = regFile->getIntReg<int64_t>(phys_int_regs_in_0);
            regFile->setIntReg<int64_t>(phys_int_regs_out_0, src_1 >> imm_value);
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
        {
            const int32_t src_1        = regFile->getIntReg<int32_t>(phys_int_regs_in_0);
            const int32_t imm_value_32 = static_cast<int32_t>(imm_value);

            regFile->setIntReg<int32_t>(phys_int_regs_out_0, src_1 >> imm_value_32);
        } break;
        default:
        {
            flagError();
        } break;
        }
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        instOp(regFile, phys_int_regs_out_0, phys_int_regs_in_0);
        log(output, 16, 65535, phys_int_regs_out_0, phys_int_regs_in_0);
        markExecuted();
    }

protected:
    int64_t imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
