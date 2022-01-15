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

#ifndef _H_VANADIS_FP_SIGN_LOGIC
#define _H_VANADIS_FP_SIGN_LOGIC

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

#include <cmath>

namespace SST {
namespace Vanadis {

enum class VanadisFPSignLogicOperation { SIGN_COPY, SIGN_XOR, SIGN_NEG };

template <typename fp_format, VanadisFPSignLogicOperation sign_op>
class VanadisFPSignLogicInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPSignLogicInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, VanadisFloatingPointFlags* fpflags, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 0, 0, 0,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1) {

		  if((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_1 + 1;
            isa_fp_regs_in[2]  = src_2;
            isa_fp_regs_in[3]  = src_2 + 1;
            isa_fp_regs_out[0] = dest;
            isa_fp_regs_out[1] = dest + 1;
        }
        else {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_2;
            isa_fp_regs_out[0] = dest;
        }
    }

    VanadisFPSignLogicInstruction* clone() override { return new VanadisFPSignLogicInstruction(*this); }
    VanadisFunctionalUnitType      getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
/*
        switch ( register_format ) {
        case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
            return "FP64SIGN";
        case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
            return "FP32SIGN";
        case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
            return "FPERROR";
        case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
            return "FPERROR";
        }
*/
        return "FPSIGN";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 ")",
            getInstCode(), isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1], phys_fp_regs_out[0], phys_fp_regs_in[0],
            phys_fp_regs_in[1]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
		  if(output->getVerboseLevel() >= 16 ) {
        char* int_register_buffer = new char[256];
        char* fp_register_buffer  = new char[256];

        writeIntRegs(int_register_buffer, 256);
        writeFPRegs(fp_register_buffer, 256);

        output->verbose(
            CALL_INFO, 16, 0, "Execute: (addr=0x%llx) %s int: %s / fp: %s\n", getInstructionAddress(), getInstCode(),
            int_register_buffer, fp_register_buffer);

        delete[] int_register_buffer;
        delete[] fp_register_buffer;
		  }
#endif

        fp_format src_1 = ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode())) ?
			combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]) : regFile->getFPReg<fp_format>(phys_fp_regs_in[0]);
        fp_format src_2 = ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode())) ?
			combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[2], phys_fp_regs_in[3]) : regFile->getFPReg<fp_format>(phys_fp_regs_in[1]);

		  fp_format result;

		  const bool src_1_neg = std::signbit(src_1);
        const bool src_2_neg = std::signbit(src_2);

		  switch(sign_op) {
		  case VanadisFPSignLogicOperation::SIGN_COPY:
            {
					result = std::copysign(src_1, src_2);
            } break;
            case VanadisFPSignLogicOperation::SIGN_XOR:
       	    {
					if(src_2_neg) {
						result = src_1_neg ? (-src_1) : src_1;
					} else {
						result = src_1_neg ? src_1 : (-src_1);
					}
            } break;
            case VanadisFPSignLogicOperation::SIGN_NEG:
            {
					result = -std::copysign(src_1, src_2);
            } break;
		  }

		  // Set any fp flag updates that we need
		  performFlagChecks<fp_format>(result);

		  if((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode())) {
				fractureToRegisters<fp_format>(regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], result);
		  } else {
				regFile->setFPReg<fp_format>(phys_fp_regs_out[0], result);
		  }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
