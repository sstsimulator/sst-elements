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

#ifndef _H_VANADIS_FP_SET_REG_COMPARE
#define _H_VANADIS_FP_SET_REG_COMPARE

#include "inst/vcmptype.h"
#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

namespace SST {
namespace Vanadis {

template <VanadisRegisterCompareType compare_type, typename fp_format>
class VanadisFPSetRegCompareInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPSetRegCompareInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t dest, const uint16_t src_1, const uint16_t src_2) :
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 1, 0, 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2, 0)
    {

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
            isa_fp_regs_in[0]   = src_1;
            isa_fp_regs_in[1]   = src_1 + 1;
            isa_fp_regs_in[2]   = src_2;
            isa_fp_regs_in[3]   = src_2 + 1;
            isa_int_regs_out[0] = dest;
        }
        else {
            isa_fp_regs_in[0]   = src_1;
            isa_fp_regs_in[1]   = src_2;
            isa_int_regs_out[0] = dest;
        }
    }

    VanadisFPSetRegCompareInstruction* clone() override { return new VanadisFPSetRegCompareInstruction(*this); }

    virtual VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    virtual const char* getInstCode() const override
    {
        /*
                switch ( register_format ) {
                case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
                {
                    switch ( compare_type ) {
                    case REG_COMPARE_EQ:
                        return "FP64CMPEQ";
                    case REG_COMPARE_NEQ:
                        return "FP64CMPNEQ";
                    case REG_COMPARE_LT:
                        return "FP64CMPLT";
                    case REG_COMPARE_LTE:
                        return "FP64CMPLTE";
                    case REG_COMPARE_GT:
                        return "FP64CMPGT";
                    case REG_COMPARE_GTE:
                        return "FP64CMPGTE";
                    default:
                        return "FP64CMPUKN";
                    }
                } break;
                case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
                {
                    switch ( compare_type ) {
                    case REG_COMPARE_EQ:
                        return "FP32CMPEQ";
                    case REG_COMPARE_NEQ:
                        return "FP32CMPNEQ";
                    case REG_COMPARE_LT:
                        return "FP32CMPLT";
                    case REG_COMPARE_LTE:
                        return "FP32CMPLTE";
                    case REG_COMPARE_GT:
                        return "FP32CMPGT";
                    case REG_COMPARE_GTE:
                        return "FP32CMPGTE";
                    default:
                        return "FP32CMPUKN";
                    }
                } break;
                case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
                    return "FPINT64ACMP";
                case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
                    return "FPINT32CMP";
                default:
                    return "FPCNVUNK";
                }
        */
        return "FPCMP";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s (op: %s, %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16 " / phys-out: %" PRIu16
            " phys-in: %" PRIu16 ", %" PRIu16 "\n",
            getInstCode(), convertCompareTypeToString(compare_type), "",
            /*registerFormatToString(register_format),*/ isa_int_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1],
            phys_int_regs_out[0], phys_fp_regs_in[0], phys_fp_regs_in[1]);
    }

    bool performCompare(SST::Output* output, VanadisRegisterFile* regFile)
    {
        const fp_format left_value =
            ((8 == sizeof(fp_format)) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()))
                ? combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1])
                : regFile->getFPReg<fp_format>(phys_fp_regs_in[0]);
        const fp_format right_value =
            ((8 == sizeof(fp_format)) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()))
                ? combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[2], phys_fp_regs_in[3])
                : regFile->getFPReg<fp_format>(phys_fp_regs_in[1]);

        switch ( compare_type ) {
        case REG_COMPARE_EQ:
            return (left_value == right_value);
        case REG_COMPARE_NEQ:
            return (left_value != right_value);
        case REG_COMPARE_LT:
            return (left_value < right_value);
        case REG_COMPARE_LTE:
            return (left_value <= right_value);
        case REG_COMPARE_GT:
            return (left_value > right_value);
        case REG_COMPARE_GTE:
            return (left_value >= right_value);
        default:
            output->fatal(CALL_INFO, -1, "Unknown compare type.\n");
            return false;
        }
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        if ( output->getVerboseLevel() >= 16 ) {
            char* int_register_buffer = new char[256];
            char* fp_register_buffer  = new char[256];

            writeIntRegs(int_register_buffer, 256);
            writeFPRegs(fp_register_buffer, 256);

            output->verbose(
                CALL_INFO, 16, 0, "Execute: (addr=0x%llx) %s (%s) int: %s / fp: %s\n", getInstructionAddress(),
                getInstCode(), convertCompareTypeToString(compare_type), int_register_buffer, fp_register_buffer);

            delete[] int_register_buffer;
            delete[] fp_register_buffer;
        }
#endif
        const bool compare_result = performCompare(output, regFile);
        regFile->setIntReg<uint64_t>(phys_int_regs_out[0], compare_result ? 1 : 0);

        if ( output->getVerboseLevel() >= 16 ) {
            if ( compare_result ) { output->verbose(CALL_INFO, 16, 0, "---> result: true\n"); }
            else {
                output->verbose(CALL_INFO, 16, 0, "---> result: false\n");
            }
        }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
