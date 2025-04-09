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

#ifndef _H_VANADIS_FP_CONVERT
#define _H_VANADIS_FP_CONVERT

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

template <typename src_format, typename dest_format>
class VanadisFPConvertInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPConvertInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, VanadisFloatingPointFlags* fpflags, const uint16_t fp_dest,
        const uint16_t fp_src) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0,
            ((sizeof(src_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(dest_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(src_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(dest_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1),
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 0, 0, 0,
            ((sizeof(src_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(dest_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(src_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(dest_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1)
    {
        if ( (sizeof(src_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
            isa_fp_regs_in[0] = fp_src;
            isa_fp_regs_in[1] = fp_src + 1;
        }
        else {
            isa_fp_regs_in[0] = fp_src;
        }

        if ( (sizeof(dest_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
            isa_fp_regs_out[0] = fp_dest;
            isa_fp_regs_out[1] = fp_dest + 1;
        }
        else {
            isa_fp_regs_out[0] = fp_dest;
        }
    }

    VanadisFPConvertInstruction* clone() override { return new VanadisFPConvertInstruction(*this); }
    VanadisFunctionalUnitType    getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
        /*
                switch ( input_format ) {
                case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
                {
                    switch ( output_format ) {
                    case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
                        return "F32F32CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
                        return "F32F64CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                        return "FP32I32CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                        return "FP32I64CNV";
                    }
                } break;
                case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
                {
                    switch ( output_format ) {
                    case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
                        return "F64F32CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
                        return "F64F64CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                        return "FP64I32CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                        return "F64I64CNV";
                    }
                } break;
                case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                {
                    switch ( output_format ) {
                    case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
                        return "I32F32CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
                        return "I32F64CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                        return "I32I32CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                        return "I32I64CNV";
                    }
                } break;
                case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                {
                    switch ( output_format ) {
                    case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
                        return "I64F32CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
                        return "I64F64CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                        return "I64I32CNV";
                    case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                        return "I64I64CNV";
                    }
                } break;
                }
        */
        //      return "FPCONVUNK";
        return "FPCONV";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
            getInstCode(), isa_fp_regs_out[0], phys_fp_regs_out[0], isa_fp_regs_in[0], phys_fp_regs_in[0]);
    }

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr,
                            uint16_t phys_fp_regs_out_0,uint16_t phys_fp_regs_in_0) override
    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) {
            output->verbose(
                CALL_INFO, verboselevel, 0,
                "hw_thr=%d sw_thr = %d Execute: 0x%" PRI_ADDR " %s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16
                "\n",
                getHWThread(),sw_thr, getInstructionAddress(), getInstCode(), isa_fp_regs_out[0], phys_fp_regs_out_0, isa_fp_regs_in[0],
                phys_fp_regs_in_0);
        }
        #endif
    }

    void instOp(VanadisRegisterFile* regFile, uint16_t phys_fp_regs_in_0, uint16_t phys_fp_regs_in_1,
                        uint16_t phys_fp_regs_out_0,uint16_t phys_fp_regs_out_1)
    {
        // This code is currenty structured MIPS else RISCV and other ISA's may not work
        if ( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) 
        {
            if ( sizeof(src_format) == 8 ) 
            {
                if ( sizeof(dest_format) == 8 ) 
                {
                    const src_format fp_v =
                        combineFromRegisters<src_format>(regFile, phys_fp_regs_in_0, phys_fp_regs_in_1);
                    fractureToRegisters<dest_format>(
                        regFile, phys_fp_regs_out_0, phys_fp_regs_out_1, static_cast<dest_format>(fp_v));

                    performFlagChecks<dest_format>(fp_v);
                }
                else 
                {
                    const src_format fp_v =
                        combineFromRegisters<src_format>(regFile, phys_fp_regs_in_0, phys_fp_regs_in_1);
                    regFile->setFPReg<dest_format>(phys_fp_regs_out_0, static_cast<dest_format>(fp_v));
                    performFlagChecks<dest_format>(fp_v);
                }
            }
            else 
            {
                if ( sizeof(dest_format) == 8 ) 
                {
                    const src_format fp_v = regFile->getFPReg<src_format>(phys_fp_regs_in_0);
                    fractureToRegisters<dest_format>(
                        regFile, phys_fp_regs_out_0, phys_fp_regs_out_1, static_cast<dest_format>(fp_v));
                    performFlagChecks<dest_format>(fp_v);
                }
                else 
                {
                    const src_format fp_v = regFile->getFPReg<src_format>(phys_fp_regs_in_0);
                    regFile->setFPReg<dest_format>(phys_fp_regs_out_0, static_cast<dest_format>(fp_v));
                    performFlagChecks<dest_format>(fp_v);
                }
            }
        } 
        else 
        {
            const src_format fp_v = regFile->getFPReg<src_format>(phys_fp_regs_in_0);

            if( isNaN( fp_v ) ) 
            {
                regFile->setFPReg<dest_format>(phys_fp_regs_out_0, NaN<dest_format>());
            } 
            else 
            {
                if ( 8 == regFile->getFPRegWidth() ) 
                {

                    if constexpr (std::is_same_v<src_format,float>) 
                    {
                        if constexpr (std::is_same_v<dest_format,double>) 
                        {
                            regFile->setFPReg<dest_format>(phys_fp_regs_out_0, static_cast<dest_format>(fp_v));
                        } 
                        else 
                        {
                            assert(0);
                        }
                    } 
                    else if constexpr (std::is_same_v<src_format,double>) 
                    {
                        if constexpr (std::is_same_v<dest_format,float>) 
                        {
                            dest_format tmp = static_cast<dest_format>(fp_v);
                            uint64_t result = 0xffffffff00000000 | convertTo<uint32_t>(tmp);
                            regFile->setFPReg<uint64_t>(phys_fp_regs_out_0, result);
                        } 
                        else 
                        {
                            assert(0);
                        }
                    } 
                    else 
                    {
                        assert(0);
                    }
                    
                } 
                else 
                {
                    assert(0);
                }
            }
            performFlagChecks<dest_format>(fp_v);
        }
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_fp_regs_out_0 = getPhysFPRegOut(0);
        uint16_t phys_fp_regs_in_0 = getPhysFPRegIn(0);
        uint16_t phys_fp_regs_in_1 = 0;
        uint16_t phys_fp_regs_out_1 = 0;
        log(output, 16, 65535, phys_fp_regs_out_0, phys_fp_regs_in_0);

        if ( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) 
        {
            if ( sizeof(src_format) == 8 ) 
            {
                phys_fp_regs_in_1 = getPhysFPRegIn(1);
            }
            if ( sizeof(dest_format) == 8 ) 
            {
                phys_fp_regs_out_1 = getPhysFPRegOut(1);
            }
        }
        instOp(regFile, phys_fp_regs_in_0, phys_fp_regs_in_1, 
                        phys_fp_regs_out_0, phys_fp_regs_out_1);
        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
