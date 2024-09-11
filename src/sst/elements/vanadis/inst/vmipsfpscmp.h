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

#ifndef _H_VANADIS_MIPS_FP_SET_REG_COMPARE
#define _H_VANADIS_MIPS_FP_SET_REG_COMPARE

#include "inst/vcmptype.h"
#include "inst/vinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

#define VANADIS_MIPS_FP_COMPARE_BIT         0x00800000
#define VANADIS_MIPS_FP_COMPARE_BIT_INVERSE 0xFF7FFFFF

namespace SST {
namespace Vanadis {

template <VanadisRegisterCompareType compare_type, typename fp_format>
class VanadisMIPSFPSetRegCompareInstruction : public VanadisInstruction
{
public:
    VanadisMIPSFPSetRegCompareInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 0, 0, 0,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 5 : 3, 1,
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 5 : 3, 1)
    {

        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode()) ) {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_1 + 1;
            isa_fp_regs_in[2]  = src_2;
            isa_fp_regs_in[3]  = src_2 + 1;
            isa_fp_regs_in[4]  = dest;
            isa_fp_regs_out[0] = dest;
        }
        else {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_2;
            isa_fp_regs_in[2]  = dest;
            isa_fp_regs_out[0] = dest;
        }
    }

    VanadisMIPSFPSetRegCompareInstruction* clone() override { return new VanadisMIPSFPSetRegCompareInstruction(*this); }

    virtual VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    virtual const char* getInstCode() const override
    {
        return "FPCMP-MO32";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        if ( VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode() ) {
            snprintf(
                buffer, buffer_size,
                "%s (op: %s, %s) isa-out: %" PRIu16 " isa-in: (%" PRIu16 ", %" PRIu16 "), (%" PRIu16 ", %" PRIu16
                ") / phys-out: %" PRIu16 " phys-in: %" PRIu16 ", %" PRIu16 "\n",
                getInstCode(), convertCompareTypeToString(compare_type), "",
                /*registerFormatToString(register_format),*/ isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1],
                isa_fp_regs_in[2], isa_fp_regs_in[3], phys_fp_regs_out[0], phys_fp_regs_in[0], phys_fp_regs_in[1]);
        }
        else {
            snprintf(
                buffer, buffer_size,
                "%s (op: %s, %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16 " / phys-out: %" PRIu16
                " phys-in: %" PRIu16 ", %" PRIu16 "\n",
                getInstCode(), convertCompareTypeToString(compare_type), "",
                /*registerFormatToString(register_format),*/ isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1],
                phys_fp_regs_out[0], phys_fp_regs_in[0], phys_fp_regs_in[1]);
        }
    }

    bool performCompare(SST::Output* output, VanadisRegisterFile* regFile, 
                        uint16_t phys_fp_regs_in_0, uint16_t phys_fp_regs_in_1,
                        uint16_t phys_fp_regs_in_2, uint16_t phys_fp_regs_in_3)
    {
        const fp_format left_value =
            ((8 == sizeof(fp_format)) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()))
                ? combineFromRegisters<fp_format>(regFile, phys_fp_regs_in_0, phys_fp_regs_in_1)
                : regFile->getFPReg<fp_format>(phys_fp_regs_in_0);
        const fp_format right_value =
            ((8 == sizeof(fp_format)) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()))
                ? combineFromRegisters<fp_format>(regFile, phys_fp_regs_in_2, phys_fp_regs_in_3)
                : regFile->getFPReg<fp_format>(phys_fp_regs_in_1);

        if ( output->getVerboseLevel() >= 16 ) {
            std::ostringstream ss;
            ss << "---> fp-values: left: " << left_value << " / right: " << right_value;
            output->verbose( CALL_INFO, 16, 0, "%s\n", ss.str().c_str());
        }

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
        case REG_COMPARE_ULT:
            return std::isnan(left_value) | std::isnan(right_value) | (left_value < right_value);
        default:
            output->fatal(CALL_INFO, -1, "Unknown compare type.\n");
            return false;
        }
    }

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr, 
                            uint16_t cond_reg_out,uint16_t cond_reg_in,
                                    bool compare_result, uint32_t cond_val)
    {
        #ifdef VANADIS_BUILD_DEBUG
            output->verbose(
                CALL_INFO, 16, 0, "Execute: 0x%" PRI_ADDR " %s (%s, %s) condition register in: %" PRIu16 " out: %" PRIu16" result: %s val=%ld \n", getInstructionAddress(), getInstCode(),
                convertCompareTypeToString(compare_type), (sizeof(fp_format) == 8) ? "64b" : "32b", cond_reg_in, cond_reg_out, compare_result==true ? 'true':'false', cond_val);
        #endif   
    }

    void instOp(SST::Output* output,VanadisRegisterFile* regFile, 
        uint16_t phys_fp_regs_out_0, uint16_t phys_fp_regs_in_0, 
        uint16_t phys_fp_regs_in_1, uint16_t phys_fp_regs_in_2, 
        uint16_t phys_fp_regs_in_3, uint16_t phys_fp_regs_in_4, 
        bool * compare_result, uint16_t* cond_reg_in, uint32_t* cond_val)
    {       
        *compare_result = performCompare(output, regFile,phys_fp_regs_in_0,phys_fp_regs_in_1,
                        phys_fp_regs_in_2,phys_fp_regs_in_3);

        *cond_reg_in =
            ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()))
                ? phys_fp_regs_in_4
                : phys_fp_regs_in_2;

        *cond_val = (regFile->getFPReg<uint32_t>(*cond_reg_in) & VANADIS_MIPS_FP_COMPARE_BIT_INVERSE);

        if ( *compare_result ) 
        {
            // true, keep everything else the same and set the compare bit to 1
            *cond_val = (*cond_val | VANADIS_MIPS_FP_COMPARE_BIT);
        }
    }
    
    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_fp_regs_out_0 = getPhysFPRegOut(0);
        uint16_t phys_fp_regs_in_0 = getPhysFPRegIn(0);
        uint16_t phys_fp_regs_in_1 = getPhysFPRegIn(1);
        uint16_t phys_fp_regs_in_2 = getPhysFPRegIn(2);
        uint16_t phys_fp_regs_in_3 = 0;
        uint16_t phys_fp_regs_in_4 = 0;
        if ( (sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()) )
        {
            uint16_t phys_fp_regs_in_3 = getPhysFPRegIn(3);
            uint16_t phys_fp_regs_in_4 = getPhysFPRegIn(4);
        }
        uint16_t cond_reg_in = 0;
        bool compare_result = false;
        uint32_t cond_val = 0;

        instOp(output, regFile,phys_fp_regs_out_0, phys_fp_regs_in_0, 
                phys_fp_regs_in_1, phys_fp_regs_in_2, phys_fp_regs_in_3, phys_fp_regs_in_4, 
                &compare_result, &cond_reg_in, &cond_val);
        log(output, 16, 65535, phys_fp_regs_out_0, cond_reg_in, compare_result,cond_val);
        regFile->setFPReg<uint32_t>(phys_fp_regs_out_0, cond_val);
        markExecuted();
    }
};


} // namespace Vanadis
} // namespace SST

#endif
