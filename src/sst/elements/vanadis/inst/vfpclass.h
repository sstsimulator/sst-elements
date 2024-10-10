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

#ifndef _H_VANADIS_FP_CLASS
#define _H_VANADIS_FP_CLASS

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

//new VanadisFPClassInstruction<uint64_t,double>(ins_address, hw_thr, options, fpflags, rd, rs1));

template <typename gpr_format, typename fp_format>
class VanadisFPClassInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPClassInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, VanadisFloatingPointFlags* fpflags,
            const uint16_t int_dest, const uint16_t fp_src ) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 1, 0, 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1, 0),
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

    VanadisFPClassInstruction* clone() override { return new VanadisFPClassInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }

    const char* getInstCode() const override
    {
        return "FPCLASS";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        #if 0
        snprintf(
            buffer, buffer_size,
            "%s int-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
            getInstCode(), isa_int_regs_out[0], phys_int_regs_out[0], isa_fp_regs_in[0], phys_fp_regs_in[0]);
        #endif
    }

    void instOp(VanadisRegisterFile* regFile, uint16_t phys_fp_regs_in_0, uint16_t phys_fp_regs_in_1,
                        uint16_t phys_int_regs_out_0)
    {
        uint64_t src;
        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) ) {
            src= combineFromRegisters<uint64_t>(regFile, phys_fp_regs_in_0, phys_fp_regs_in_1);
        } else {
            if ( 8 == regFile->getFPRegWidth() ) {
                src = regFile->getFPReg<uint64_t>(phys_fp_regs_in_0);
                if constexpr ( 4 == sizeof( fp_format ) ) { 
                    assert( isNaN_boxed( src ) );
                }
            } else {
                src = regFile->getFPReg<uint32_t>(phys_fp_regs_in_0);
            }
        }
        int shift;
        auto fp_v = int64To<fp_format>(src); 

        switch ( std::fpclassify( fp_v ) ) {
            case FP_INFINITE:
                if ( std::signbit( fp_v ) ) {
                    shift = 0; 
                } else {
                    shift = 7; 
                }
                break;
            case FP_NAN:
                if ( isNaNs( fp_v ) ) {
                    shift = 8; 
                } else {
                    shift = 9; 
                }
                break;
            case FP_NORMAL:
                if ( std::signbit( fp_v ) ) {
                    shift = 1; 
                } else {
                    shift = 6; 
                }
                break;
            case FP_SUBNORMAL:
                if ( std::signbit( fp_v ) ) {
                    shift = 2; 
                } else {
                    shift = 5; 
                }
                break;
            case FP_ZERO:
                if ( std::signbit( fp_v ) ) {
                    shift = 3; 
                } else {
                    shift = 4; 
                }
                break;
            default: assert(0);
        }

        gpr_format flags = 1 << shift;

        regFile->setIntReg<gpr_format>(phys_int_regs_out_0, flags);
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        uint16_t phys_fp_regs_in_0 = getPhysFPRegIn(0);
        uint16_t phys_fp_regs_in_1 = getPhysFPRegIn(1);
        log(output, 16, 65535, phys_int_regs_out_0, phys_fp_regs_in_0, phys_fp_regs_in_1);
        instOp(regFile, phys_fp_regs_in_0, phys_fp_regs_in_1, phys_int_regs_out_0);
        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
