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

#ifndef _H_VANADIS_FP_2_FP
#define _H_VANADIS_FP_2_FP

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

template <typename fp_format>
class VanadisFP2FPInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFP2FPInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t fp_dest, const uint16_t fp_src) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 0, 0, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1)
    {

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
            isa_fp_regs_out[0] = fp_dest;
            isa_fp_regs_out[1] = fp_dest + 1;
            isa_fp_regs_in[0]  = fp_src;
            isa_fp_regs_in[1]  = fp_src + 1;
        }
        else {
            isa_fp_regs_out[0] = fp_dest;
            isa_fp_regs_in[0]  = fp_src;
        }
    }

    VanadisFP2FPInstruction*  clone() override { return new VanadisFP2FPInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
        if ( 8 == sizeof(fp_format) ) {
				return "FP642FP64";
		  }
        else if ( 4 == sizeof(fp_format) ) {
            return "FP322FP32";
        }
        else {
            return "FP2FP";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16 "\n",
            getInstCode(), isa_fp_regs_out[0], phys_fp_regs_out[0], isa_fp_regs_in[0], phys_fp_regs_in[0]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: 0x%llx %s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16 " phys: %" PRIu16
            "\n",
            getInstructionAddress(), getInstCode(), isa_fp_regs_out[0], phys_fp_regs_out[0], isa_fp_regs_in[0],
            phys_fp_regs_in[0]);
#endif

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) ) {
            const int32_t v_0 = regFile->getFPReg<int32_t>(phys_fp_regs_in[0]);
            regFile->setFPReg<int32_t>(phys_fp_regs_out[0], v_0);

            const int32_t v_1 = regFile->getFPReg<int32_t>(phys_fp_regs_in[1]);
            regFile->setFPReg<int32_t>(phys_fp_regs_out[1], v_1);
        }
        else {
            const fp_format fp_v = regFile->getFPReg<fp_format>(phys_fp_regs_in[0]);
            regFile->setFPReg<fp_format>(phys_fp_regs_out[0], fp_v);
        }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
