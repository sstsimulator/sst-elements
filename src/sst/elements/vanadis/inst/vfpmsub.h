// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_FP_MSUB
#define _H_VANADIS_FP_MSUB

#include <cfenv>

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

// template <VanadisRegisterFormat register_format>
template <typename fp_format>
class VanadisFPFusedMultiplySubInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPFusedMultiplySubInstruction(
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

    VanadisFPFusedMultiplySubInstruction*  clone() override { return new VanadisFPFusedMultiplySubInstruction(*this); }
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
                CALL_INFO, 16, 0, "Execute: 0x%llx %s\n", getInstructionAddress(), getInstCode());
        }
#endif

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) ) {
            const fp_format src_1  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]);
            const fp_format src_2  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[2], phys_fp_regs_in[3]);
            const fp_format src_3  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[4], phys_fp_regs_in[5]);
            const fp_format result = std::fma( (fp_format) src_1, (fp_format) src_2, (fp_format) -src_3 ); 

            performMsubFlagChecks<fp_format>(src_1,src_2,src_3);
            performFlagChecks<fp_format>(result);

            if ( output->getVerboseLevel() >= 16 ) {
                output->verbose(CALL_INFO, 0, 0, "---> %f * %f + %f= %f\n", src_1, src_2, src_3, result);
                output->verbose(CALL_INFO, 16, 0, "---> %f * %f + %f= %f\n", src_1, src_2, src_3, result);
            }

            fractureToRegisters<fp_format>(regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], result);
        }
        else {
            const fp_format src_1  = regFile->getFPReg<fp_format>(phys_fp_regs_in[0]);
            const fp_format src_2  = regFile->getFPReg<fp_format>(phys_fp_regs_in[1]);
            const fp_format src_3  = regFile->getFPReg<fp_format>(phys_fp_regs_in[2]);
            const fp_format result = std::fma( (fp_format) src_1, (fp_format) src_2, (fp_format) -src_3 ); 

            performMsubFlagChecks<fp_format>(src_1,src_2,src_3);
            performFlagChecks<fp_format>(result);

            if ( output->getVerboseLevel() >= 16 ) {
                output->verbose(CALL_INFO, 16, 0, "---> %f * %f + %f = %f\n", src_1, src_2, src_3, result);
            }

            regFile->setFPReg<fp_format>(phys_fp_regs_out[0], result);
        }

        markExecuted();
    }
    template <typename T>
    void performMsubFlagChecks(const T src_1, const T src_2, const T src_3)
    {
        if ( std::fpclassify(src_1) == FP_ZERO || std::fpclassify(src_2) == FP_ZERO ||
            std::fpclassify(src_1) == FP_INFINITE || std::fpclassify(src_2) == FP_INFINITE ||
            std::fpclassify(src_1) == FP_NAN || std::fpclassify(src_2) == FP_NAN ||
            src_3 == FP_NAN || src_3 == FP_INFINITE ) {
            fpflags.setInvalidOp();
            update_fp_flags = true;
        }
    }

};

} // namespace Vanadis
} // namespace SST

#endif
