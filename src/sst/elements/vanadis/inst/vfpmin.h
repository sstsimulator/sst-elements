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

#ifndef _H_VANADIS_FP_MIN
#define _H_VANADIS_FP_MIN

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

template <typename fp_format, bool is_min >
class VanadisFPMinimumInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPMinimumInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t dest, const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 0, 0, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1),
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

    VanadisFPMinimumInstruction* clone() override { return new VanadisFPMinimumInstruction(*this); }
    VanadisFunctionalUnitType     getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
        if ( std::is_same<fp_format, double>::value ) { return "FP64MIN"; }
        else if ( std::is_same<fp_format, float>::value ) {
            return "FP32MIN";
        }
        else if ( std::is_same<fp_format, int32_t>::value ) {
            return "FPI32MIN";
        }
        else if ( std::is_same<fp_format, uint32_t>::value ) {
            return "FPU32MIN";
        }
        else if ( std::is_same<fp_format, int64_t>::value ) {
            return "FPI64MIN";
        }
        else if ( std::is_same<fp_format, uint64_t>::value ) {
            return "FPU64MIN";
        }
        else {
            return "FPMINNUNK";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%6s  %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 ")",
            getInstCode(), isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1], phys_fp_regs_out[0],
            phys_fp_regs_in[0], phys_fp_regs_in[1]);
    }

    void instOp(VanadisRegisterFile* regFile, uint16_t phys_fp_regs_in_0,
                        uint16_t phys_fp_regs_in_1, uint16_t phys_fp_regs_in_2,
                        uint16_t phys_fp_regs_in_3,
                        uint16_t phys_fp_regs_out_0,uint16_t phys_fp_regs_out_1)
    {
         if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) )
         {
                fp_format src_1,src_2;
                src_1  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in_0, phys_fp_regs_in_1);
                src_2  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in_2, phys_fp_regs_in_3);

                fp_format result;
                if constexpr  ( is_min )
                {
                    result = src_1 < src_2 ? src_1 : src_2;
                }
                else
                {
                    result = src_1 > src_2 ? src_1 : src_2;
                }

                performFlagChecks<fp_format>(result);

                fractureToRegisters<fp_format>(regFile, phys_fp_regs_out_0, phys_fp_regs_out_1, result);
        }
        else
        {
            const fp_format src_1  = regFile->getFPReg<fp_format>(phys_fp_regs_in_0);
            const fp_format src_2  = regFile->getFPReg<fp_format>(phys_fp_regs_in_1);

            fp_format result;
            if constexpr  ( is_min ) {
                if ( UNLIKELY( ( src_1 == -0.0 && src_2 == 0.0 ) || ( src_1 == 0.0 && src_2 == -0.0 ) ) ) {
                    result = -0.0;
                } else {
                    result = src_1 < src_2 ? src_1 : src_2;
                }
            } else {
                if ( UNLIKELY( ( src_1 == -0.0 && src_2 == 0.0 ) || ( src_1 == 0.0 && src_2 == -0.0 ) ) ) {
                    result = 0.0;
                } else if ( UNLIKELY( isNaN( src_1 ) && isNaN( src_2 ) ) ) {
                    result = NaN<fp_format>();
                } else if ( UNLIKELY( isNaN( src_1 ) ) ) {
                    result = src_2;
                    fpflags.setInvalidOp();
                    update_fp_flags = true;
                } else {
                    result = src_1 > src_2 ? src_1 : src_2;
                }
            }

            performFlagChecks<fp_format>(result);

            regFile->setFPReg<fp_format>(phys_fp_regs_out_0, result);
        }
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
         uint16_t phys_fp_regs_out_0 = getPhysFPRegOut(0);
        uint16_t phys_fp_regs_in_0 = getPhysFPRegIn(0);
        uint16_t phys_fp_regs_in_1 = getPhysFPRegIn(1);
        uint16_t phys_fp_regs_in_2 = 0;
        uint16_t phys_fp_regs_in_3 = 0;
        uint16_t phys_fp_regs_out_1 = 0;
        #ifdef VANADIS_BUILD_DEBUG
        log(output, 16, 65535, phys_fp_regs_out_0, phys_fp_regs_in_0, phys_fp_regs_in_1);
        #endif

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) )
        {
            phys_fp_regs_in_2 = getPhysFPRegIn(2);
            phys_fp_regs_in_3 = getPhysFPRegIn(3);
            phys_fp_regs_out_1 = getPhysFPRegOut(1);
        }
        instOp(regFile, phys_fp_regs_in_0, phys_fp_regs_in_1,
                        phys_fp_regs_in_2, phys_fp_regs_in_3,
                        phys_fp_regs_out_0,phys_fp_regs_out_1);
        markExecuted();
    }
};



} // namespace Vanadis
} // namespace SST

#endif
