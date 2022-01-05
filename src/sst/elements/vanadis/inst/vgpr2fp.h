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
#include "util/vtypename.h"

#include <vector>

namespace SST {
namespace Vanadis {

// template <VanadisRegisterFormat int_register_format, VanadisRegisterFormat fp_register_format>
template <typename gpr_format, typename fp_format>
class VanadisGPR2FPInstruction : public VanadisInstruction
{
public:
    VanadisGPR2FPInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t fp_dest,
        const uint16_t int_src) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 1, 0, 1, 0, 0,
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

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: 0x%llx %s fp-dest isa: %" PRIu16 " phys: %" PRIu16 " <- int-src: isa: %" PRIu16
            " phys: %" PRIu16 "\n",
            getInstructionAddress(), getInstCode(), isa_fp_regs_out[0], phys_fp_regs_out[0], isa_int_regs_in[0],
            phys_int_regs_in[0]);
#endif

        const gpr_format v = regFile->getIntReg<gpr_format>(phys_int_regs_in[0]);
		  const fp_format  v_fp = static_cast<fp_format>(v);

		  if(output->getVerboseLevel() >= 16 ) {
				output->verbose(CALL_INFO, 16, 0, "----> convert: ");

				if(std::is_same<gpr_format, int32_t>::value) {
					output->verbose(CALL_INFO, 16, 0, "%" PRId32 " -> %f\n", v, v_fp);
				} else if(std::is_same<gpr_format, uint32_t>::value) {
               output->verbose(CALL_INFO, 16, 0, "%" PRIu32 " -> %f\n", v, v_fp);
            } else if(std::is_same<gpr_format, int64_t>::value) {
               output->verbose(CALL_INFO, 16, 0, "%" PRId64 " -> %f\n", v, v_fp);
            } else if(std::is_same<gpr_format, uint64_t>::value) {
               output->verbose(CALL_INFO, 16, 0, "%" PRIu64 " -> %f\n", v, v_fp);
            }
		  }

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) ) {
            fractureToRegisters<fp_format>(
                regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], v_fp);
        }
        else {
            regFile->setFPReg<fp_format>(phys_fp_regs_out[0], v_fp);
        }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
