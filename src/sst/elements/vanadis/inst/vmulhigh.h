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

#ifndef _H_VANADIS_MUL_HIGH
#define _H_VANADIS_MUL_HIGH

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template< typename gpr_format_1, typename gpr_format_2 >
class VanadisMultiplyHighInstruction : public VanadisInstruction
{
public:
    VanadisMultiplyHighInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0)
    {
        assert( sizeof(gpr_format_1) == sizeof(uint64_t) );
        assert( sizeof(gpr_format_2) == sizeof(uint64_t) );

        isa_int_regs_in[0]  = src_1;
        isa_int_regs_in[1]  = src_2;
        isa_int_regs_out[0] = dest;
    }

    VanadisMultiplyHighInstruction* clone() override { return new VanadisMultiplyHighInstruction(*this); }

    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char*               getInstCode() const override
    {
        if( std::is_signed<gpr_format_1>::value && std::is_signed<gpr_format_2>::value ) {
           return "MULH";
        }
        if( ! std::is_signed<gpr_format_1>::value && ! std::is_signed<gpr_format_2>::value ) {
           return "MULHU";
        }
        if( std::is_signed<gpr_format_1>::value && ! std::is_signed<gpr_format_2>::value ) {
           return "MULHSU";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s   %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 ")",
            getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_in[0],
            phys_int_regs_in[1]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= 16) {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: 0x%llx %s phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16
                " / in=%" PRIu16 ", %" PRIu16 "\n",
                getInstructionAddress(), getInstCode(), phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1],
                isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1]);
        }
#endif

        uint64_t result;

        // "MULH";
        if ( std::is_signed<gpr_format_1>::value && std::is_signed<gpr_format_2>::value ) {
            const __int128_t src_1 = regFile->getIntReg<gpr_format_1>(phys_int_regs_in[0]);
            const __int128_t src_2 = regFile->getIntReg<gpr_format_2>(phys_int_regs_in[1]);
            result = ( ( src_1 * src_2 ) >> (8 * sizeof(gpr_format_1)) );
        // "MULHU";
        } else if ( ! std::is_signed<gpr_format_1>::value && ! std::is_signed<gpr_format_2>::value ) {
            const __uint128_t src_1 = regFile->getIntReg<gpr_format_1>(phys_int_regs_in[0]);
            const __uint128_t src_2 = regFile->getIntReg<gpr_format_2>(phys_int_regs_in[1]);
            result = ( ( src_1 * src_2 ) >> (8 * sizeof(gpr_format_1)) );
        // "MULHSU";
        } else if ( std::is_signed<gpr_format_1>::value && ! std::is_signed<gpr_format_2>::value ) {
            const __int128_t src_1 = regFile->getIntReg<gpr_format_1>(phys_int_regs_in[0]);
            const __uint128_t src_2 = regFile->getIntReg<gpr_format_2>(phys_int_regs_in[1]);
            result = ( ( src_1 * src_2 ) >> (8 * sizeof(gpr_format_1)) );
        } else {
            assert(0);
        }
        
        regFile->setIntReg<uint64_t>( phys_int_regs_out[0], result );

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
