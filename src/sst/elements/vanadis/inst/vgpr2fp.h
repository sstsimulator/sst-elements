// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_GPR_2_FP
#define _H_VANADIS_GPR_2_FP

#include "inst/vinst.h"
#include "inst/vregfmt.h"

namespace SST {
namespace Vanadis {

template <VanadisRegisterFormat int_register_format, VanadisRegisterFormat fp_register_format>
class VanadisGPR2FPInstruction : public VanadisInstruction
{
public:
    VanadisGPR2FPInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t fp_dest,
        const uint16_t int_src) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 1, 0, 1, 0, 0,
            ((fp_register_format == VanadisRegisterFormat::VANADIS_FORMAT_FP64 ||
              fp_register_format == VanadisRegisterFormat::VANADIS_FORMAT_INT64) &&
             (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()))
                ? 2
                : 1,
            0,
            ((fp_register_format == VanadisRegisterFormat::VANADIS_FORMAT_FP64 ||
              fp_register_format == VanadisRegisterFormat::VANADIS_FORMAT_INT64) &&
             (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()))
                ? 2
                : 1)
    {
        isa_int_regs_in[0] = int_src;

        if ( ((fp_register_format == VanadisRegisterFormat::VANADIS_FORMAT_FP64 ||
               fp_register_format == VanadisRegisterFormat::VANADIS_FORMAT_INT64) &&
              (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ) {

            isa_fp_regs_out[0] = fp_dest;
            isa_fp_regs_out[1] = fp_dest + 1;
        }
        else {
            isa_fp_regs_out[0] = fp_dest;
        }
    }

    VanadisGPR2FPInstruction* clone() override { return new VanadisGPR2FPInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }

    const char* getInstCode() const override
    {
        switch ( fp_register_format ) {
        case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
            switch(int_register_format)
            {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                return "GPR32FP32";
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                return "GPR64FP32";
            }
            break;
        case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
            switch(int_register_format)
            {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                return "GPR32FP32";
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                return "GPR64FP32";
            }
            break;
        case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
            switch(int_register_format)
            {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                return "GPR32FP64";
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                return "GPR64FP64";
            }
            break;
        case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
            switch(int_register_format)
            {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                return "GPR32FP64";
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                return "GPR64FP64";
            }
            break;
        }

        return "GPRCONVUNK";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- int-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
            getInstCode(), isa_fp_regs_out[0], phys_fp_regs_out[0], isa_int_regs_in[0], phys_int_regs_in[0]);
    }

    template <typename FP_TYPE, typename INT_TYPE>
    void execute_inner(SST::Output* output, VanadisRegisterFile* regFile)
    {
        INT_TYPE v = regFile->getIntReg<INT_TYPE>(phys_int_regs_in[0]);

        // Data type is 8 bytes wide, but we are in 4 byte register width mode
        if ( (8 == sizeof(FP_TYPE)) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) ) {
            fractureToRegisters<FP_TYPE>(regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], static_cast<FP_TYPE>(v));
        }
        else {
            regFile->setFPReg<FP_TYPE>(phys_fp_regs_out[0], static_cast<FP_TYPE>(v));
        }
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute (addr=0x%llx) %s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- int-src: isa: %" PRIu16
            " phys: %" PRIu16 "\n",
            getInstructionAddress(), getInstCode(), isa_fp_regs_out[0], phys_fp_regs_out[0], isa_int_regs_in[0],
            phys_int_regs_in[0]);
#endif
        switch ( fp_register_format ) {
        case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
        {
            switch ( int_register_format ) {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
            {
                execute_inner<int32_t, int32_t>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
            {
                execute_inner<int32_t, int64_t>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
            {
                execute_inner<int32_t, float>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
            {
                execute_inner<int32_t, double>(output, regFile);
            } break;
            }
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
        {
            switch ( int_register_format ) {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
            {
                execute_inner<float, int32_t>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
            {
                execute_inner<float, int64_t>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
            {
                execute_inner<float, float>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
            {
                execute_inner<float, double>(output, regFile);
            } break;
            }
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
        {
            switch ( int_register_format ) {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
            {
                execute_inner<int64_t, int32_t>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
            {
                execute_inner<int64_t, int64_t>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
            {
                execute_inner<int64_t, float>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
            {
                execute_inner<int64_t, double>(output, regFile);
            } break;
            }
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
        {
            switch ( int_register_format ) {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
            {
                execute_inner<double, int32_t>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
            {
                execute_inner<double, int64_t>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
            {
                execute_inner<double, float>(output, regFile);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
            {
                execute_inner<double, double>(output, regFile);
            } break;
            }
        } break;
        }
        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
