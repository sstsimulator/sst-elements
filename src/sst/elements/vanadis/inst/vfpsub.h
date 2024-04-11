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

#ifndef _H_VANADIS_FP_SUB
#define _H_VANADIS_FP_SUB

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

template <typename fp_format>
class VanadisFPSubInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPSubInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, VanadisFloatingPointFlags* fpflags, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 0, 0, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1)
    {

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_1 + 1;
            isa_fp_regs_in[2]  = src_2;
            isa_fp_regs_in[3]  = src_2 + 1;
            isa_fp_regs_out[0] = dest;
            isa_fp_regs_out[1] = dest + 1;
        }
        else {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_2;
            isa_fp_regs_out[0] = dest;
        }
    }

    VanadisFPSubInstruction*  clone() override { return new VanadisFPSubInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
        if ( std::is_same<fp_format, double>::value ) { return "FP64SUB"; }
        else if ( std::is_same<fp_format, float>::value ) {
            return "FP32SUB";
        }
        else if ( std::is_same<fp_format, int32_t>::value ) {
            return "FPI32SUB";
        }
        else if ( std::is_same<fp_format, uint32_t>::value ) {
            return "FPU32SUB";
        }
        else if ( std::is_same<fp_format, int64_t>::value ) {
            return "FPI64SUB";
        }
        else if ( std::is_same<fp_format, uint64_t>::value ) {
            return "FPU64SUB";
        }
        else {
            return "FPSUBUNK";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%6s  %5" PRIu16 " <- %5" PRIu16 " - %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " - %5" PRIu16 ")",
            getInstCode(), isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1], phys_fp_regs_out[0],
            phys_fp_regs_in[0], phys_fp_regs_in[1]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= 16) {
            output->verbose(
                CALL_INFO, 16, 0, "Execute: (addr=0x%" PRI_ADDR ") %s\n", getInstructionAddress(), getInstCode());
        }
#endif
        if constexpr (! ( std::is_same_v<fp_format,float> || std::is_same_v<fp_format,double> ) ) { assert(0); }

        clear_IEEE754_except();

        if ( sizeof(fp_format) >= regFile->getFPRegWidth() ) {

            fp_format src_1,src_2,result;
            READ_2FP_REGS;

            if ( UNLIKELY( ! std::isfinite( src_1 ) || ! std::isfinite( src_2 ) ) ) {
                result = NaN<fp_format>();
                fpflags.setInvalidOp();
                update_fp_flags = true;
            } else {
                result = src_1 - src_2;
            }

            performFlagChecks<fp_format>(result);

            WRITE_FP_REGS;

        } else {

            const uint64_t src_1  = regFile->getFPReg<uint64_t>(phys_fp_regs_in[0]);
            const uint64_t src_2  = regFile->getFPReg<uint64_t>(phys_fp_regs_in[1]);
            uint64_t result;

            assert( isNaN_boxed( src_1 ) );
            assert( isNaN_boxed( src_2 ) );

            const auto fp_1 = int64To<float>(src_1 );
            const auto fp_2 = int64To<float>(src_2 );

            if ( UNLIKELY( ! std::isfinite(fp_1) || ! std::isfinite(fp_2) ) ) {
                result = NaN<uint32_t>();
                fpflags.setInvalidOp();
                update_fp_flags = true;
            } else {
                auto tmp = fp_1 - fp_2;
                performFlagChecks<float>(tmp);
                result = convertTo<int64_t>(tmp);
            }
            result |= 0xffffffff00000000;

            regFile->setFPReg<uint64_t>(phys_fp_regs_out[0], result);
        }

        check_IEEE754_except();
        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
