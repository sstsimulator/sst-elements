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

#ifndef _H_VANADIS_GPR_2_FP
#define _H_VANADIS_GPR_2_FP

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
//#include "util/vtypename.h"

#include <vector>

namespace SST {
namespace Vanadis {

// template <VanadisRegisterFormat int_register_format, VanadisRegisterFormat fp_register_format>
template <typename gpr_format, typename fp_format, bool is_bitwise >
class VanadisGPR2FPInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisGPR2FPInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, VanadisFloatingPointFlags* fpflags, const uint16_t fp_dest,
        const uint16_t int_src) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 1, 0, 1, 0, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1),
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 1, 0, 1, 0, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1)
    {
        isa_int_regs_in[0] = int_src;

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
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
        //		constexpr v_constexpr_str<3> gp_type_name = vanadis_type_name<gpr_format>();
        //		constexpr v_constexpr_str<3> fp_type_name = vanadis_type_name<fp_format>();

        //		return (gp_type_name + fp_type_name).data();
        return "GPR2FP";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- int-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
            getInstCode(), isa_fp_regs_out[0], phys_fp_regs_out[0], isa_int_regs_in[0], phys_int_regs_in[0]);
    }

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr,
                            uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0) override
        {
            #ifdef VANADIS_BUILD_DEBUG
            if(output->getVerboseLevel() >= verboselevel) {
                output->verbose(
                CALL_INFO, verboselevel, 0,
                "hw_thr=%d sw_thr = %d Execute: 0x%" PRI_ADDR " %s phys: out=%" PRIu16 " in=%" PRIu16 ", isa: out=%" PRIu16
                " / in=%" PRIu16 "\n",
                getHWThread(),sw_thr, getInstructionAddress(), getInstCode(), phys_int_regs_out_0, phys_int_regs_in_0,
                 isa_fp_regs_out[0], isa_int_regs_in[0]);
            }
            #endif
        }

    inline void bitwise_convert(VanadisRegisterFile* regFile,
                                uint16_t phys_fp_regs_out_0, uint16_t phys_fp_regs_out_1,
                                uint16_t phys_int_regs_in_0 )
    {

        const gpr_format v    = regFile->getIntReg<gpr_format>(phys_int_regs_in_0);
        uint64_t result;


        if constexpr ( std::is_same_v<fp_format,uint64_t> && std::is_same_v<gpr_format,uint64_t> ) {
            result = v;
        } else if constexpr ( std::is_same_v<fp_format,uint32_t> && std::is_same_v<gpr_format,uint32_t> ) {
            if ( 8 == regFile->getFPRegWidth() ) {
                result = 0xffffffff00000000 | v ;
            } else {
                result = v;
            }
        } else {
            assert(0);
        }

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) ) {
            fractureToRegisters<uint64_t>(regFile, phys_fp_regs_out_0, phys_fp_regs_out_1, result);
        } else {
            if ( 8 == regFile->getFPRegWidth() ) {
                regFile->setFPReg<uint64_t>(phys_fp_regs_out_0, result);
            } else {
                regFile->setFPReg<uint32_t>(phys_fp_regs_out_0, static_cast<uint32_t>(result));
            }
        }
    }

    void convert(VanadisRegisterFile* regFile,
                                uint16_t phys_fp_regs_out_0, uint16_t phys_fp_regs_out_1,
                                uint16_t phys_int_regs_in_0)
    {

        const gpr_format v       = regFile->getIntReg<gpr_format>(phys_int_regs_in_0);
        const fp_format  result  = static_cast<fp_format>(v);

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) ) {
            fractureToRegisters<fp_format>(regFile, phys_fp_regs_out_0, phys_fp_regs_out_1, result);
        } else {
            if( 8 == regFile->getFPRegWidth() && 4 == sizeof(fp_format) ) {
                uint64_t tmp = 0xffffffff00000000 | convertTo<uint64_t>(result);
                regFile->setFPReg<uint64_t>(phys_fp_regs_out_0, tmp);
            } else {
                regFile->setFPReg<fp_format>(phys_fp_regs_out_0, result);
            }
        }

        if (UNLIKELY(isa_int_regs_in[0] != isa_options->getRegisterIgnoreWrites())) {
            performFlagChecks<fp_format>(result);
        }

    }

    void instOp(VanadisRegisterFile* regFile,
                                uint16_t phys_fp_regs_out_0, uint16_t phys_fp_regs_out_1,
                                uint16_t phys_int_regs_in_0) override
    {

        if constexpr ( is_bitwise ) {
            bitwise_convert(regFile,phys_fp_regs_out_0, phys_fp_regs_out_1, phys_int_regs_in_0 );
        } else {
            convert(regFile,phys_fp_regs_out_0, phys_fp_regs_out_1, phys_int_regs_in_0 );
        }

    }
    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_fp_regs_out_0 = getPhysFPRegOut(0);
        uint16_t phys_fp_regs_out_1 = 0;
        uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);
        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) )
        {
            phys_fp_regs_out_1 = getPhysFPRegOut(1);
        }
        instOp(regFile, phys_fp_regs_out_0, phys_fp_regs_out_1, phys_int_regs_in_0);
        #ifdef VANADIS_BUILD_DEBUG
        log(output,16, 65535, phys_fp_regs_out_0,phys_int_regs_in_0);
        #endif
        markExecuted();
    }
};


} // namespace Vanadis
} // namespace SST

#endif
