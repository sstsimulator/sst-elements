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

#ifndef _H_VANADIS_FP_SQRT
#define _H_VANADIS_FP_SQRT

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

template <typename fp_format>
class VanadisFPSquareRootInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPSquareRootInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t dest, const uint16_t src_1) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 0, 0, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1)
    {

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_1 + 1;
            isa_fp_regs_out[0] = dest;
            isa_fp_regs_out[1] = dest + 1;
        }
        else {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_out[0] = dest;
        }
    }

    VanadisFPSquareRootInstruction* clone() override { return new VanadisFPSquareRootInstruction(*this); }
    VanadisFunctionalUnitType     getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
        if ( std::is_same<fp_format, double>::value ) { return "FP64SQRT"; }
        if ( std::is_same<fp_format, float>::value ) { return "FP32SQRT"; }

        return "FPUNK";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) ) {
            assert(0);
        } else {
            snprintf( buffer, buffer_size, "%6s  %5" PRIu16 " <- %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 ")", getInstCode(),
                    isa_fp_regs_out[0], isa_fp_regs_in[0], phys_fp_regs_out[0], phys_fp_regs_in[0]);
        }
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if ( output->getVerboseLevel() >= 16 ) {
            output->verbose(
                CALL_INFO, 16, 0, "Execute: (addr=0x%" PRI_ADDR ") %s\n", getInstructionAddress(),
                getInstCode());
        }
#endif
        clear_IEEE754_except();

        if ( sizeof(fp_format) >= regFile->getFPRegWidth() ) {

            fp_format src_1;
            READ_FP_REG;

            fp_format result = std::sqrt(src_1);
            performFlagChecks<fp_format>(result);

            if ( isNaN(result) ) {
                result = NaN<fp_format>();
            }   

            WRITE_FP_REGS;

        } else {

            uint64_t src_1  = regFile->getFPReg<uint64_t>(phys_fp_regs_in[0]);

            assert( isNaN_boxed( src_1 ) );

            float tmp = std::sqrt( int64To<float>(src_1) );

            performFlagChecks<float>(tmp);

            uint64_t result = 0xffffffff00000000;

            if ( UNLIKELY( isNaN(tmp) ) ) {
                float i = NaN<float>();
                result |= *(uint32_t*) &i;
            } else {
                result |= *(uint32_t*) &tmp;
            }
            
            regFile->setFPReg<uint64_t>(phys_fp_regs_out[0], result);
        }

        check_IEEE754_except();

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
