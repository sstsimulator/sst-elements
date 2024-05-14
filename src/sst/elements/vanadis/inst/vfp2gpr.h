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

#ifndef _H_VANADIS_FP_2_GPR
#define _H_VANADIS_FP_2_GPR

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

template <typename fp_format, typename gpr_format, bool is_bitwise>
class VanadisFP2GPRInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFP2GPRInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, VanadisFloatingPointFlags* fpflags, const uint16_t int_dest,
        const uint16_t fp_src) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 1, 0, 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1, 0)
    {

        isa_int_regs_out[0] = int_dest;

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
            isa_fp_regs_in[0] = fp_src;
            isa_fp_regs_in[1] = fp_src + 1;
        }
        else {
            isa_fp_regs_in[0] = fp_src;
        }
    }

    VanadisFP2GPRInstruction* clone() override { return new VanadisFP2GPRInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }

    const char* getInstCode() const override
    {
        /*
                switch ( register_format ) {
                case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
                    return "FP2GPR32";
                case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
                    return "FP2GPR64";
                }
        */
        //        return "FPGPRUNK";
        return "FP2GPR";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s int-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
            getInstCode(), isa_int_regs_out[0], phys_int_regs_out[0], isa_fp_regs_in[0], phys_fp_regs_in[0]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= 16) {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: 0x%" PRI_ADDR " %s int-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16
                "\n",
                getInstructionAddress(), getInstCode(), isa_int_regs_out[0], phys_int_regs_out[0], isa_fp_regs_in[0],
                phys_fp_regs_in[0]);
        }
#endif
        if constexpr ( is_bitwise ) {
            bitwise_convert( output, regFile );
        } else {
            convert( output, regFile );
        }
        markExecuted();
    }

    void bitwise_convert( SST::Output* output, VanadisRegisterFile* regFile)
    { 
        if  constexpr ( std::is_floating_point<gpr_format>::value ) {
            assert(0);
        }

        uint64_t fp_v;
        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) ) {
            fp_v = combineFromRegisters<uint64_t>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]);
        } else {
            if ( 8 == regFile->getFPRegWidth() ) {
                fp_v = regFile->getFPReg<uint64_t>(phys_fp_regs_in[0]);
            } else {
                fp_v = regFile->getFPReg<uint32_t>(phys_fp_regs_in[0]);
            }
        }

        if constexpr ( std::is_same_v<gpr_format,uint32_t> ) {

            if ( isSignBitSet( static_cast<uint32_t>(fp_v) ) ) {
                fp_v |= 0xffffffff00000000;
            } else {
                fp_v &= 0x00000000ffffffff;
            }
        }

        regFile->setIntReg<uint64_t>(phys_int_regs_out[0], fp_v);
    }

    void convert( SST::Output* output, VanadisRegisterFile* regFile)
    { 
        uint64_t reg_v_int;
        gpr_format result;
        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) ) {
            reg_v_int = combineFromRegisters<uint64_t>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]);
        } else {
            if ( 8 == regFile->getFPRegWidth() ) {
                reg_v_int = regFile->getFPReg<uint64_t>(phys_fp_regs_in[0]);
            } else {
                reg_v_int = regFile->getFPReg<uint32_t>(phys_fp_regs_in[0]);
            }
        }

        auto fp_v = *(fp_format*) &reg_v_int;

        if ( UNLIKELY( isInf(fp_v) || isNaN(fp_v) ) ) {
            result = getMaxInt<gpr_format>();
        } else if ( UNLIKELY( fp_v - 1.0 >= getMaxInt<gpr_format>() ) ) {
            fpflags.setInvalidOp();
            update_fp_flags = true;
            result = getMaxInt<gpr_format>();
        } else if ( UNLIKELY( fp_v + 1.0 <= getMinInt<gpr_format>() ) ) {
            fpflags.setInvalidOp();
            update_fp_flags = true;
            result = getMinInt<gpr_format>();
        } else { 
            result = fp_v;
            if ( fp_v != result ) {
                fpflags.setInexact();
                update_fp_flags = true;
            }
        }

        regFile->setIntReg<gpr_format>(phys_int_regs_out[0], result);
    }
};

} // namespace Vanadis
} // namespace SST

#endif
