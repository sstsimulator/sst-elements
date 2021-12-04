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

#include <cmath>

#include "inst/vinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

enum class VanadisFPSignLogicOperation
{
    SIGN_COPY,
    SIGN_XOR,
	 SIGN_NEG
};

template <VanadisRegisterFormat register_format, VanadisFPSignLogicOperation sign_op>
class VanadisFPSignLogicInstruction : public VanadisInstruction
{
public:
    VanadisFPSignLogicInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 0, 0, 0,
            ((register_format == VanadisRegisterFormat::VANADIS_FORMAT_FP64) &&
             (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()))
                ? 4
                : 2,
            ((register_format == VanadisRegisterFormat::VANADIS_FORMAT_FP64) &&
             (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()))
                ? 2
                : 1,
            ((register_format == VanadisRegisterFormat::VANADIS_FORMAT_FP64) &&
             (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()))
                ? 4
                : 2,
            ((register_format == VanadisRegisterFormat::VANADIS_FORMAT_FP64) &&
             (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()))
                ? 2
                : 1)
    {

        if ( (register_format == VanadisRegisterFormat::VANADIS_FORMAT_FP64) &&
             (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
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

        return "FPERROR";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "MUL     %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 ")",
            isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1], phys_fp_regs_out[0], phys_fp_regs_in[0],
            phys_fp_regs_in[1]);
    }

    template <typename T>
    void perform_sign_copy(SST::Output* output, VanadisRegisterFile* regFile)
    {
        T src_1 = regFile->getFPReg<T>(phys_fp_regs_in[0]);
        T src_2 = regFile->getFPReg<T>(phys_fp_regs_in[1]);

        if ( std::isnormal(src_1) || std::isnormal(src_2) ) {
            // true is negative
				const bool src_1_neg = std::signbit(src_1);
				const bool src_2_neg = std::signbit(src_2);

            if ( src_2_neg ) {
					src_1 = src_1_neg ? src_1 : (-src_1);
				} else {
					src_1 = src_1_neg ? (-src_1) : (src_1);
				}

            output->verbose(CALL_INFO, 16, 0, "---> take-sign-from: %f to: %f = %f\n", src_2, src_1, src_1);

            regFile->setFPReg<T>(phys_fp_regs_out[0], src_1);
        }
    }

    template <typename T>
    void perform_sign_neg(SST::Output* output, VanadisRegisterFile* regFile)
    {
        T src_1 = regFile->getFPReg<T>(phys_fp_regs_in[0]);
        T src_2 = regFile->getFPReg<T>(phys_fp_regs_in[1]);

        if ( std::isnormal(src_1) || std::isnormal(src_2) ) {
            // true is negative
				const bool src_1_neg = std::signbit(src_1);
				const bool src_2_neg = std::signbit(src_2);

            if ( src_2_neg ) {
					src_1 = src_1_neg ? (-src_1) : (src_1);
				} else {
					src_1 = src_1_neg ? (src_1) : (-src_1);
				}

            output->verbose(CALL_INFO, 16, 0, "---> neg-sign-from: %f to: %f = %f\n", src_2, src_1, src_1);

            regFile->setFPReg<T>(phys_fp_regs_out[0], src_1);
        }
    }

    template <typename T>
    void perform_sign_xor(SST::Output* output, VanadisRegisterFile* regFile)
    {
        T src_1 = regFile->getFPReg<T>(phys_fp_regs_in[0]);
        T src_2 = regFile->getFPReg<T>(phys_fp_regs_in[1]);

        if ( std::isnormal(src_1) || std::isnormal(src_2) ) {
            // true is negative
				const bool src_1_neg = std::signbit(src_1);
				const bool src_2_neg = std::signbit(src_2);

            if ( src_1_neg ^ src_2_neg ) {
					src_1 = (src_1 > 0) ? (-src_1) : src_1;
				} else {
					src_1 = (src_1 > 0) ? src_1 : (-src_1);
				}

            output->verbose(CALL_INFO, 16, 0, "---> xor-sign-from: %f and %f = %f\n", src_2, src_1, src_1);

            regFile->setFPReg<T>(phys_fp_regs_out[0], src_1);
        }
    }

    template <typename T>
    void perform_sign_copy_split(SST::Output* output, VanadisRegisterFile* regFile)
    {
        T src_1 = combineFromRegisters<T>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]);
        T src_2 = combineFromRegisters<T>(regFile, phys_fp_regs_in[2], phys_fp_regs_in[3]);

        if ( std::isnormal(src_1) || std::isnormal(src_2) ) {
            // true is negative
				const bool src_1_neg = std::signbit(src_1);
				const bool src_2_neg = std::signbit(src_2);

            if ( src_2_neg ) {
					src_1 = src_1_neg ? src_1 : (-src_1);
				} else {
					src_1 = src_1_neg ? (-src_1) : (src_1);
				}

            output->verbose(CALL_INFO, 16, 0, "---> take-sign-from: %f to: %f = %f\n", src_2, src_1, src_1);

            fractureToRegisters<T>(regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], src_1);
        }
    }

    template <typename T>
    void perform_sign_neg_split(SST::Output* output, VanadisRegisterFile* regFile)
    {
        T src_1 = combineFromRegisters<T>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]);
        T src_2 = combineFromRegisters<T>(regFile, phys_fp_regs_in[2], phys_fp_regs_in[3]);

        if ( std::isnormal(src_1) || std::isnormal(src_2) ) {
            // true is negative
				const bool src_1_neg = std::signbit(src_1);
				const bool src_2_neg = std::signbit(src_2);

            if ( src_2_neg ) {
					src_1 = src_1_neg ? (-src_1) : (src_1);
				} else {
					src_1 = src_1_neg ? (src_1) : (-src_1);
				}

            output->verbose(CALL_INFO, 16, 0, "---> neg-sign-from: %f to: %f = %f\n", src_2, src_1, src_1);

            fractureToRegisters<T>(regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], src_1);
        }
    }

    template <typename T>
    void perform_sign_xor_split(SST::Output* output, VanadisRegisterFile* regFile)
    {
        T src_1 = combineFromRegisters<T>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]);
        T src_2 = combineFromRegisters<T>(regFile, phys_fp_regs_in[2], phys_fp_regs_in[3]);

        if ( std::isnormal(src_1) || std::isnormal(src_2) ) {
            // true is negative
				const bool src_1_neg = std::signbit(src_1);
				const bool src_2_neg = std::signbit(src_2);

            if ( src_1_neg ^ src_2_neg ) {
					src_1 = (src_1 > 0) ? (-src_1) : src_1;
				} else {
					src_1 = (src_1 > 0) ? src_1 : (-src_1);
				}

            output->verbose(CALL_INFO, 16, 0, "---> xor-sign-from: %f and %f = %f\n", src_2, src_1, src_1);

            fractureToRegisters<T>(regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], src_1);
        }
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        char* int_register_buffer = new char[256];
        char* fp_register_buffer  = new char[256];

        writeIntRegs(int_register_buffer, 256);
        writeFPRegs(fp_register_buffer, 256);

        output->verbose(
            CALL_INFO, 16, 0, "Execute: (addr=0x%llx) %s int: %s / fp: %s\n", getInstructionAddress(), getInstCode(),
            int_register_buffer, fp_register_buffer);

        delete[] int_register_buffer;
        delete[] fp_register_buffer;
#endif

        switch ( register_format ) {
        case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
        {
            switch ( sign_op ) {
            case VanadisFPSignLogicOperation::SIGN_COPY:
            {
                perform_sign_copy<float>(output, regFile);
            } break;
				case VanadisFPSignLogicOperation::SIGN_XOR:
				{
					perform_sign_xor<float>(output, regFile);
				} break;
				case VanadisFPSignLogicOperation::SIGN_NEG:
				{
					perform_sign_neg<float>(output, regFile);
				} break;
            }
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
        {
            switch ( sign_op ) {
            case VanadisFPSignLogicOperation::SIGN_COPY:
            {
                if ( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
                    perform_sign_copy_split<double>(output, regFile);
                }
                else {
                    perform_sign_copy<double>(output, regFile);
                }
            } break;
				case VanadisFPSignLogicOperation::SIGN_XOR:
				{
					if ( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
						perform_sign_xor_split<double>(output, regFile);
					} else {
						perform_sign_xor<double>(output, regFile);
					}
				} break;
				case VanadisFPSignLogicOperation::SIGN_NEG:
				{
					if ( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
						perform_sign_neg_split<double>(output, regFile);
					} else {
						perform_sign_neg<double>(output, regFile);
					}
				} break;
            }
        } break;
        default:
        {
            output->verbose(CALL_INFO, 16, 0, "Unknown floating point type.\n");
            flagError();
        } break;
        }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
