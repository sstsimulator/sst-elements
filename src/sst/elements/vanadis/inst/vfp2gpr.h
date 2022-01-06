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

#ifndef _H_VANADIS_FP_2_GPR
#define _H_VANADIS_FP_2_GPR

#include "inst/vinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

template<typename fp_format, typename gpr_format>
class VanadisFP2GPRInstruction : public VanadisInstruction
{
public:
    VanadisFP2GPRInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t int_dest,
        const uint16_t fp_src) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 1, 0, 1,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            0,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
            0)
    {

        isa_int_regs_out[0] = int_dest;

		  if((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) {
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
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: 0x%llx %s int-dest isa: %" PRIu16 " phys: %" PRIu16 " <- fp-src: isa: %" PRIu16
            " phys: %" PRIu16 "\n",
            getInstructionAddress(), getInstCode(), isa_int_regs_out[0], phys_int_regs_out[0], isa_fp_regs_in[0],
            phys_fp_regs_in[0]);
#endif
		  if((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode())) {
                const fp_format fp_v = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]);
                regFile->setIntReg<gpr_format>(phys_int_regs_out[0], static_cast<gpr_format>(fp_v));
		  } else {
				const fp_format fp_v = regFile->getFPReg<fp_format>(phys_fp_regs_in[0]);
				regFile->setIntReg<gpr_format>(phys_int_regs_out[0], static_cast<gpr_format>(fp_v));
		  }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
