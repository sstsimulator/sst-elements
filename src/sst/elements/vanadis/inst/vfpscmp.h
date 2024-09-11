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
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 1, 0, 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2, 0),
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

    bool performCompare(SST::Output* output, VanadisRegisterFile* regFile, uint16_t phys_fp_regs_in_0,
                        uint16_t phys_fp_regs_in_1,uint16_t phys_fp_regs_in_2,uint16_t phys_fp_regs_in_3)
    {
        uint64_t left_value_int;
        uint64_t right_value_int;

        if ( ( 8 == sizeof(fp_format)) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode())){
            left_value_int = combineFromRegisters<uint64_t>(regFile, phys_fp_regs_in_0, phys_fp_regs_in_1);
            right_value_int =combineFromRegisters<uint64_t>(regFile, phys_fp_regs_in_2, phys_fp_regs_in_3);
        } else {
            if ( 8 == regFile->getFPRegWidth() ) {
                left_value_int = regFile->getFPReg<uint64_t>(phys_fp_regs_in_0);
                right_value_int = regFile->getFPReg<uint64_t>(phys_fp_regs_in_1);
            } else {
                left_value_int = regFile->getFPReg<uint32_t>(phys_fp_regs_in_0);
                right_value_int = regFile->getFPReg<uint32_t>(phys_fp_regs_in_1);
            }
        }

        if constexpr (std::is_same_v<fp_format,float>) {
            if ( 8 == regFile->getFPRegWidth() ) {
                // if the upper 32 bits are not all 1's then these NaN
                if ( ! ( isNaN_boxed( left_value_int ) && isNaN_boxed( right_value_int ) ) ) {
                    fpflags.setInvalidOp();
                    update_fp_flags = true;
                    return 0;
                }
            }
        }

        auto left_value = *(fp_format*) & left_value_int; 
        auto right_value = *(fp_format*) & right_value_int; 

        // assuming these are signalling comparison units
        // check both units for NaN
        performFlagChecks<fp_format>(left_value);
        performFlagChecks<fp_format>(right_value);

        switch ( compare_type ) {
        case REG_COMPARE_EQ:
            if ( UNLIKELY( isNaNs( left_value ) || isNaNs( right_value ) ) ) {
                fpflags.setInvalidOp();
                update_fp_flags = true;
            }
            return (left_value == right_value);
        case REG_COMPARE_NEQ:
            return (left_value != right_value);
        case REG_COMPARE_LT:
            if ( UNLIKELY( isNaN( left_value ) || isNaN( right_value ) || isNaNs( left_value ) || isNaNs( right_value ) ) )
            {

                fpflags.setInvalidOp();
                update_fp_flags = true;
            }
            return (left_value < right_value);
        case REG_COMPARE_LTE:
            if ( UNLIKELY( isNaN( left_value ) || isNaN( right_value ) || isNaNs( left_value ) || isNaNs( right_value ) ) )
            {
                fpflags.setInvalidOp();
                update_fp_flags = true;
            }
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

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr, 
                            uint16_t phys_fp_regs_out_0,uint16_t phys_fp_regs_in_0,
                                    uint16_t phys_fp_regs_in_1, uint16_t compare_result)
    {
        #ifdef VANADIS_BUILD_DEBUG
        if ( output->getVerboseLevel() >= 16 ) {
            output->verbose(
                CALL_INFO, 16, 0, "hw_thr=%d sw_thr = %d Execute: (addr=0x%" PRI_ADDR ") %s (%s) phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16
                    " / in=%" PRIu16 ", %" PRIu16 ", %" PRIu16 "  result: %s\n", getHWThread(),sw_thr, getInstructionAddress(), getInstCode(), 
                    convertCompareTypeToString(compare_type), phys_fp_regs_out_0, phys_fp_regs_in_0,
                    phys_fp_regs_in_1, isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1], compare_result ? "true" : "false");
        }
        #endif
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        uint16_t phys_fp_regs_in_0 = getPhysFPRegIn(0);
        uint16_t phys_fp_regs_in_1 = getPhysFPRegIn(1);
        uint16_t phys_fp_regs_in_2 = 0;
        uint16_t phys_fp_regs_in_3 = 0;
        
        if ( ( 8 == sizeof(fp_format)) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()))
        {
            phys_fp_regs_in_2 = getPhysFPRegIn(2);
            phys_fp_regs_in_3 = getPhysFPRegIn(3);
        }
        
        
        const bool compare_result = performCompare(output, regFile,phys_fp_regs_in_0, phys_fp_regs_in_1, phys_fp_regs_in_2,
                                                    phys_fp_regs_in_3);
        regFile->setIntReg<uint64_t>(phys_int_regs_out_0, compare_result ? 1 : 0);
        log(output, 16,65535, phys_int_regs_out_0, phys_fp_regs_in_0,
                                    phys_fp_regs_in_1, compare_result);
        markExecuted();
    }
};


template <VanadisRegisterCompareType compare_type, typename fp_format>
class VanadisSIMTFPSetRegCompareInstruction : public VanadisSIMTInstruction, public VanadisFPSetRegCompareInstruction<compare_type,fp_format>
{
public:
    VanadisSIMTFPSetRegCompareInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t dest, const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 1, 0, 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2, 0),
        VanadisSIMTInstruction(
            addr, hw_thr, isa_opts, 0, 1, 0, 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2, 0),
        VanadisFPSetRegCompareInstruction<compare_type,fp_format>(addr, hw_thr, isa_opts, fpflags, dest, src_1, src_2)
    {
        ;
    }

    VanadisSIMTFPSetRegCompareInstruction* clone() override { return new VanadisSIMTFPSetRegCompareInstruction(*this); }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0, VanadisSIMTInstruction::sw_thread);
        uint16_t phys_fp_regs_in_0 = getPhysFPRegIn(0, VanadisSIMTInstruction::sw_thread);
        uint16_t phys_fp_regs_in_1 = getPhysFPRegIn(1, VanadisSIMTInstruction::sw_thread);
        uint16_t phys_fp_regs_in_2 = 0;
        uint16_t phys_fp_regs_in_3 = 0;
        
        if ( ( 8 == sizeof(fp_format)) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()))
        {
            phys_fp_regs_in_2 = getPhysFPRegIn(2, VanadisSIMTInstruction::sw_thread);
            phys_fp_regs_in_3 = getPhysFPRegIn(3, VanadisSIMTInstruction::sw_thread);
        }
        
        
        const bool compare_result = this->performCompare(output, regFile,phys_fp_regs_in_0, phys_fp_regs_in_1, phys_fp_regs_in_2,
                                                    phys_fp_regs_in_3);
        regFile->setIntReg<uint64_t>(phys_int_regs_out_0, compare_result ? 1 : 0);
        VanadisFPSetRegCompareInstruction<compare_type,fp_format>::log(output, 16, VanadisSIMTInstruction::sw_thread, phys_int_regs_out_0, phys_fp_regs_in_0,
                                    phys_fp_regs_in_1, compare_result);
    }
};

} // namespace Vanadis
} // namespace SST

#endif
