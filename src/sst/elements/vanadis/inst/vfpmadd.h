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

#ifndef _H_VANADIS_FP_MADD
#define _H_VANADIS_FP_MADD

#include <cfenv>

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

// template <VanadisRegisterFormat register_format>
template <typename fp_format, bool rs1_is_neg>
class VanadisFPFusedMultiplyAddInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPFusedMultiplyAddInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t dest, const uint16_t src_1, const uint16_t src_2, const uint64_t src_3) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 0, 0, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 6 : 3,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 3 : 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 6 : 3,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 3 : 1)
    {
        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_1 + 1;
            isa_fp_regs_in[2]  = src_2;
            isa_fp_regs_in[3]  = src_2 + 1;
            isa_fp_regs_in[4]  = src_3;
            isa_fp_regs_in[5]  = src_3 + 1;
            isa_fp_regs_out[0] = dest;
            isa_fp_regs_out[1] = dest + 1;
        }
        else {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_2;
            isa_fp_regs_in[2]  = src_3;
            isa_fp_regs_out[0] = dest;
        }
    }

    VanadisFPFusedMultiplyAddInstruction*  clone() override { return new VanadisFPFusedMultiplyAddInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
        if ( std::is_same<fp_format, double>::value ) {
			return "FP64MADD";
		  }
        else if ( std::is_same<fp_format, float>::value ) {
            return "FP32MADD";
        }
        else {
            return "FPMADDUNK";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s  %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 " + %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 " + %5" PRIu16 ")",
            getInstCode(), isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1], isa_fp_regs_in[2], phys_fp_regs_out[0],
            phys_fp_regs_in[0], phys_fp_regs_in[1], phys_fp_regs_in[2]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if ( output->getVerboseLevel() >= 16 ) {
            output->verbose(
                CALL_INFO, 16, 0, "Execute: 0x%" PRI_ADDR " %s\n", getInstructionAddress(), getInstCode());
        }
#endif

        clear_IEEE754_except();

        if ( sizeof(fp_format) >= regFile->getFPRegWidth() ) {

            fp_format src_1,src_2, src_3;
            READ_3FP_REGS;

            performMaddFlagChecks<fp_format>(src_1,src_2,src_3);

            fp_format result = std::fma( (fp_format) src_1, (fp_format) src_2, (fp_format) src_3 ); 
            if ( rs1_is_neg ) {
                result *= -1.0;
            }

            performFlagChecks<fp_format>(result);

            performFlagChecks<fp_format>(result);

            WRITE_FP_REGS;

        } else {

            const uint64_t src_1  = regFile->getFPReg<uint64_t>(phys_fp_regs_in[0]);
            const uint64_t src_2  = regFile->getFPReg<uint64_t>(phys_fp_regs_in[1]);
            const uint64_t src_3  = regFile->getFPReg<uint64_t>(phys_fp_regs_in[2]);

            assert( isNaN_boxed( src_1 ) );
            assert( isNaN_boxed( src_2 ) );
            assert( isNaN_boxed( src_3 ) );

            float fp_1 = int64To<float>(src_1);
            float fp_2 = int64To<float>(src_2);
            float fp_3 = int64To<float>(src_3);
            performMaddFlagChecks<fp_format>(fp_1,fp_2,fp_3);

            fp_format tmp = std::fma( (fp_format) fp_1, (fp_format) fp_2, (fp_format) fp_3 ); 
            if ( rs1_is_neg ) {
                tmp *= -1.0;
            }

            performFlagChecks<fp_format>(tmp);

            const uint64_t result = 0xffffffff00000000 | convertTo<int64_t>(tmp);

            regFile->setFPReg<uint64_t>(phys_fp_regs_out[0], result);
        }

        check_IEEE754_except();

        markExecuted();
    }

    template <typename T>
    void performMaddFlagChecks(const T src_1, const T src_2, const T src_3)
    {

        if ( std::fpclassify(src_1) == FP_ZERO || 
            std::fpclassify(src_1) == FP_INFINITE || 
            std::fpclassify(src_1) == FP_NAN ) { 
            fpflags.setInvalidOp();
            update_fp_flags = true;
        } else if ( std::fpclassify(src_2) == FP_ZERO ||
            std::fpclassify(src_2) == FP_INFINITE ||
            std::fpclassify(src_2) == FP_NAN ) { 
            fpflags.setInvalidOp();
            update_fp_flags = true;
        } else if ( std::fpclassify(src_3) == FP_NAN || 
            std::fpclassify(src_3) == FP_INFINITE ) {
            fpflags.setInvalidOp();
            update_fp_flags = true;
        }
    }

};

} // namespace Vanadis
} // namespace SST

#endif
