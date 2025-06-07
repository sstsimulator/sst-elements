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

#ifndef _H_VANADIS_MUL_SPLIT
#define _H_VANADIS_MUL_SPLIT

#include "inst/vinst.h"

#include <cstdint>
#include <ios>
#include <sstream>

namespace SST {
namespace Vanadis {

template <typename register_format>
class VanadisMultiplySplitInstruction : public VanadisInstruction
{
public:
    VanadisMultiplySplitInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest_lo,
        const uint16_t dest_hi, const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 2, 2, 2, 0, 0, 0, 0)
    {
        isa_int_regs_in[0]  = src_1;
        isa_int_regs_in[1]  = src_2;
        isa_int_regs_out[0] = dest_lo;
        isa_int_regs_out[1] = dest_hi;
    }

    VanadisMultiplySplitInstruction* clone() override { return new VanadisMultiplySplitInstruction(*this); }
    VanadisFunctionalUnitType        getInstFuncType() const override { return INST_INT_ARITH; }

    const char* getInstCode() const override {
        switch(sizeof(register_format)) {
        case 8:
            return "MULSPLIT64";
        case 4:
            return "MULSPLIT32";
        case 2:
            return "MULSPLIT16";
        default:
            return "MULSPLIT";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s lo: %5" PRIu16 " hi: %" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 " (phys: lo: %5" PRIu16
            " hi: %" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 ")",
            getInstCode(), isa_int_regs_out[0], isa_int_regs_out[1], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0],
            phys_int_regs_out[1], phys_int_regs_in[0], phys_int_regs_in[1]);
    }


    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        #ifdef VANADIS_BUILD_DEBUG
                if(output->getVerboseLevel() >= 16) {
                    output->verbose(
                        CALL_INFO, 16, 0,
                        "Execute: (addr=%p) %s phys: out-lo: %" PRIu16 " out-hi: %" PRIu16 " in=%" PRIu16 ", %" PRIu16
                        ", isa: out-lo: %" PRIu16 " out-hi: %" PRIu16 " / in=%" PRIu16 ", %" PRIu16 "\n",
                        (void*)getInstructionAddress(), getInstCode(),
                        phys_int_regs_out[0], phys_int_regs_out[1], phys_int_regs_in[0],
                        phys_int_regs_in[1], isa_int_regs_out[0], isa_int_regs_out[1], isa_int_regs_in[0], isa_int_regs_in[1]);
                }
        #endif
        const register_format src_1 = regFile->getIntReg<register_format>(phys_int_regs_in[0]);
        const register_format src_2 = regFile->getIntReg<register_format>(phys_int_regs_in[1]);

        constexpr int64_t lo_mask = (4 == sizeof(register_format)) ?
                0xFFFFFFFF :
                (2 == sizeof(register_format)) ?
                0xFFFF : 0xFF;

        if(std::is_signed<register_format>::value) {
            const int64_t multiply_result = (src_1) * (src_2);
            const register_format result_lo       = (register_format)(multiply_result & lo_mask);
            const register_format result_hi       = (register_format)(multiply_result >> (sizeof(register_format) * 8));

            if(output->getVerboseLevel() >= 16) {
                std::ostringstream ss;
                ss << "-> Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
                ss << " " << src_1 <<" * " << src_2 << " = "<< multiply_result << " = (lo: " << result_lo << ", hi: " << result_hi;
                output->verbose( CALL_INFO, 16, 0, "%s\n", ss.str().c_str());
            }

            regFile->setIntReg<register_format>(phys_int_regs_out[0], result_lo);
            regFile->setIntReg<register_format>(phys_int_regs_out[1], result_hi);
        } else {
            const uint64_t multiply_result = (src_1) * (src_2);
            const register_format result_lo       = (register_format)(multiply_result & lo_mask);
            const register_format result_hi       = (register_format)(multiply_result >> (sizeof(register_format) * 8));

            if(output->getVerboseLevel() >= 16) {
                std::ostringstream ss;
                ss << "-> Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
                ss << " " << src_1 <<" * " << src_2 << " = "<< multiply_result << " = (lo: " << result_lo << ", hi: " << result_hi;
                output->verbose( CALL_INFO, 16, 0, "%s\n", ss.str().c_str());
            }

            regFile->setIntReg<register_format>(phys_int_regs_out[0], result_lo);
            regFile->setIntReg<register_format>(phys_int_regs_out[1], result_hi);
        }

        markExecuted();
    }


};

} // namespace Vanadis
} // namespace SST

#endif
