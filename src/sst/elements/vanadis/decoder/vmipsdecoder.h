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

#ifndef _H_VANADIS_MIPS_DECODER
#define _H_VANADIS_MIPS_DECODER

#include "decoder/vauxvec.h"
#include "decoder/vdecoder.h"
#include "inst/isatable.h"
#include "inst/vinstall.h"
#include "os/vmipscpuos.h"
#include "util/vdatacopy.h"
#include "vinsloader.h"

#include <list>
#include <sys/types.h>
#include <unistd.h>

#define MIPS_REG_ZERO 0
#define MIPS_REG_LO   32
#define MIPS_REG_HI   33

#define MIPS_FP_VER_REG    32
#define MIPS_FP_STATUS_REG 33

#define MIPS_OP_MASK 0xFC000000
#define MIPS_RS_MASK 0x3E00000
#define MIPS_RT_MASK 0x1F0000
#define MIPS_RD_MASK 0xF800

#define MIPS_FD_MASK 0x7C0
#define MIPS_FS_MASK 0xF800
#define MIPS_FT_MASK 0x1F0000
#define MIPS_FR_MASK 0x3E00000

#define MIPS_ADDR_MASK    0x7FFFFFF
#define MIPS_J_ADDR_MASK  0x3FFFFFF
#define MIPS_J_UPPER_MASK 0xF0000000
#define MIPS_IMM_MASK     0xFFFF
#define MIPS_SHFT_MASK    0x7C0
#define MIPS_FUNC_MASK    0x3F

#define MIPS_SPEC_OP_SPECIAL3 0x7c000000

#define MIPS_SPECIAL_OP_MASK 0x7FF

#define MIPS_SPEC_OP_MASK_ADD  0x20
#define MIPS_SPEC_OP_MASK_ADDU 0x21
#define MIPS_SPEC_OP_MASK_AND  0x24

#define MIPS_SPEC_OP_MASK_ANDI   0x30000000
#define MIPS_SPEC_OP_MASK_ORI    0x34000000
#define MIPS_SPEC_OP_MASK_REGIMM 0x04000000
#define MIPS_SPEC_OP_MASK_BGEZAL 0x00110000
#define MIPS_SPEC_OP_MASK_BGTZ   0x1C000000
#define MIPS_SPEC_OP_MASK_LUI    0x3C000000
#define MIPS_SPEC_OP_MASK_ADDIU  0x24000000
#define MIPS_SPEC_OP_MASK_LB     0x80000000
#define MIPS_SPEC_OP_MASK_LBU    0x90000000
#define MIPS_SPEC_OP_MASK_LL     0xC0000000
#define MIPS_SPEC_OP_MASK_LW     0x8C000000
#define MIPS_SPEC_OP_MASK_LWL    0x88000000
#define MIPS_SPEC_OP_MASK_LWR    0x98000000
#define MIPS_SPEC_OP_MASK_LHU    0x94000000
#define MIPS_SPEC_OP_MASK_SB     0xA0000000
#define MIPS_SPEC_OP_MASK_SC     0xE0000000
#define MIPS_SPEC_OP_MASK_SH     0xA4000000
#define MIPS_SPEC_OP_MASK_SW     0xAC000000
#define MIPS_SPEC_OP_MASK_SWL    0xA8000000
#define MIPS_SPEC_OP_MASK_SWR    0xB8000000
#define MIPS_SPEC_OP_MASK_BEQ    0x10000000
#define MIPS_SPEC_OP_MASK_BNE    0x14000000
#define MIPS_SPEC_OP_MASK_BLEZ   0x18000000
#define MIPS_SPEC_OP_MASK_SLTI   0x28000000
#define MIPS_SPEC_OP_MASK_SLTIU  0x2C000000
#define MIPS_SPEC_OP_MASK_JAL    0x0C000000
#define MIPS_SPEC_OP_MASK_J      0x08000000
#define MIPS_SPEC_OP_MASK_COP1   0x44000000
#define MIPS_SPEC_OP_MASK_XORI   0x38000000

#define MIPS_SPEC_OP_MASK_SFP32 0xE4000000
#define MIPS_SPEC_OP_MASK_LFP32 0xC4000000

#define MIPS_SPEC_OP_MASK_BLTZ    0x0
#define MIPS_SPEC_OP_MASK_BGEZ    0x10000
#define MIPS_SPEC_OP_MASK_BREAK   0x0D
#define MIPS_SPEC_OP_MASK_DADD    0x2C
#define MIPS_SPEC_OP_MASK_DADDU   0x2D
#define MIPS_SPEC_OP_MASK_DIV     0x1A
#define MIPS_SPEC_OP_MASK_DIVU    0x1B
#define MIPS_SPEC_OP_MASK_DDIV    0x1E
#define MIPS_SPEC_OP_MASK_DDIVU   0x1F
#define MIPS_SPEC_OP_MASK_DMULT   0x1C
#define MIPS_SPEC_OP_MASK_DMULTU  0x1D
#define MIPS_SPEC_OP_MASK_DSLL    0x38
#define MIPS_SPEC_OP_MASK_DSLL32  0x3C
#define MIPS_SPEC_OP_MASK_DSLLV   0x14
#define MIPS_SPEC_OP_MASK_DSRA    0x3B
#define MIPS_SPEC_OP_MASK_DSRA32  0x3F
#define MIPS_SPEC_OP_MASK_DSRAV   0x17
#define MIPS_SPEC_OP_MASK_DSRL    0x3A
#define MIPS_SPEC_OP_MASK_DSRL32  0x3E
#define MIPS_SPEC_OP_MASK_DSRLV   0x16
#define MIPS_SPEC_OP_MASK_DSUB    0x2E
#define MIPS_SPEC_OP_MASK_DSUBU   0x2F
#define MIPS_SPEC_OP_MASK_JALR    0x09
#define MIPS_SPEC_OP_MASK_JR      0x08
#define MIPS_SPEC_OP_MASK_MFHI    0x10
#define MIPS_SPEC_OP_MASK_MFLO    0x12
#define MIPS_SPEC_OP_MASK_MOVN    0x0B
#define MIPS_SPEC_OP_MASK_MOVZ    0x0A
#define MIPS_SPEC_OP_MASK_MTHI    0x11
#define MIPS_SPEC_OP_MASK_MTLO    0x13
#define MIPS_SPEC_OP_MASK_MULT    0x18
#define MIPS_SPEC_OP_MASK_MULTU   0x19
#define MIPS_SPEC_OP_MASK_NOR     0x27
#define MIPS_SPEC_OP_MASK_OR      0x25
#define MIPS_SPEC_OP_MASK_RDHWR   0x3B
#define MIPS_SPEC_OP_MASK_SLL     0x00
#define MIPS_SPEC_OP_MASK_SLLV    0x04
#define MIPS_SPEC_OP_MASK_SLT     0x2A
#define MIPS_SPEC_OP_MASK_SLTU    0x2B
#define MIPS_SPEC_OP_MASK_SRA     0x03
#define MIPS_SPEC_OP_MASK_SRAV    0x07
#define MIPS_SPEC_OP_MASK_SRL     0x02
#define MIPS_SPEC_OP_MASK_SRLV    0x06
#define MIPS_SPEC_OP_MASK_SUB     0x22
#define MIPS_SPEC_OP_MASK_SUBU    0x23
#define MIPS_SPEC_OP_MASK_SYSCALL 0x0C
#define MIPS_SPEC_OP_MASK_SYNC    0x0F
#define MIPS_SPEC_OP_MASK_XOR     0x26

#define MIPS_SPEC_COP_MASK_CF   0x2
#define MIPS_SPEC_COP_MASK_CT   0x6
#define MIPS_SPEC_COP_MASK_MFC  0x0
#define MIPS_SPEC_COP_MASK_MTC  0x4
#define MIPS_SPEC_COP_MASK_MOV  0x6
#define MIPS_SPEC_COP_MASK_CVTS 0x20
#define MIPS_SPEC_COP_MASK_CVTD 0x21
#define MIPS_SPEC_COP_MASK_CVTW 0x24
#define MIPS_SPEC_COP_MASK_MUL  0x2
#define MIPS_SPEC_COP_MASK_ADD  0x0
#define MIPS_SPEC_COP_MASK_SUB  0x1
#define MIPS_SPEC_COP_MASK_DIV  0x3

#define MIPS_SPEC_COP_MASK_CMP_LT  0x3C
#define MIPS_SPEC_COP_MASK_CMP_LTE 0x3E
#define MIPS_SPEC_COP_MASK_CMP_EQ  0x32

#define MIPS_INC_DECODE_STAT(stat_name) (stat_name)->addData(1);

namespace SST {
namespace Vanadis {

class VanadisMIPSDecoder : public VanadisDecoder
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VanadisMIPSDecoder, "vanadis", "VanadisMIPSDecoder",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Implements a MIPS-compatible decoder for Vanadis CPU processing.",
                                          SST::Vanadis::VanadisDecoder)

    SST_ELI_DOCUMENT_PARAMS({ "decode_max_ins_per_cycle", "Maximum number of instructions that can be "
                                                          "decoded and issued per cycle" },
                            { "uop_cache_entries", "Number of micro-op cache entries, this "
                                                   "corresponds to ISA-level instruction counts." },
                            { "predecode_cache_entries",
                              "Number of cache lines that a cached prior to decoding (these support "
                              "loading from cache prior to decode)" })

    SST_ELI_DOCUMENT_STATISTICS(
				VANADIS_DECODER_ELI_STATISTICS,
				{ "ins_decode_add", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_addu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_and", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_dadd", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_daddu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_ddiv", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_div", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_divu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_dmult", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_dmultu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_dsllv", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_dsrav", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_dsrlv", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_dsub", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_dsubu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_jr", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_jalr", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_mfhi", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_mflo", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_mult", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_multu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_nor", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_or", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sllv", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_slt", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sltu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_srav", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_srlv", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sub", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_subu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_syscall", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sync", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_xor", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sll", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_srl", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sra", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_bltz", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_bgezal", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_bgez", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_lui", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_lb", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_lbu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_lhu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_lw", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_lfp32", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_ll", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_lwl", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_lwr", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sb", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sc", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sw", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sh", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sfp32", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_swr", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_swl", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_addiu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_beq", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_bgtz", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_blez", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_bne", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_slti", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_sltiu", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_andi", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_ori", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_j", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_jal", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_xori", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_rdhwr", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_mtc", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_mfc", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_cf", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_ct", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_mov", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_mul", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_div", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_sub", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_cvts", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_cvtd", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_cvtw", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_lt", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_lte", "Count number of instructions decoded", "ins", 1 },
     { "ins_decode_cop1_eq", "Count number of instructions decoded", "ins", 1 }
			       )

    VanadisMIPSDecoder(ComponentId_t id, Params& params) : VanadisDecoder(id, params)
    {

        // 32 int + hi/lo (2) = 34
        // 32 fp + ver + status (2) = 34
        // reg-2 is for sys-call codes
        // plus 2 for LO/HI registers in INT
        options               = new VanadisDecoderOptions((uint16_t)0, 34, 34, 2, VANADIS_REGISTER_MODE_FP32);
        max_decodes_per_cycle = params.find<uint16_t>("decode_max_ins_per_cycle", 2);

        // See if we get an entry point the sub-component says we have to use
        // if not, we will fall back to ELF reading at the core level to work this
        // out
        setInstructionPointer(params.find<uint64_t>("entry_point", 0));

        haltOnDecodeZero = true;
        initStatistics();
    }

    ~VanadisMIPSDecoder() {}

    virtual const char*                  getISAName() const { return "MIPS"; }
    virtual uint16_t                     countISAIntReg() const { return options->countISAIntRegisters(); }
    virtual uint16_t                     countISAFPReg() const { return options->countISAFPRegisters(); }
    virtual const VanadisDecoderOptions* getDecoderOptions() const { return options; }

    virtual VanadisFPRegisterMode getFPRegisterMode() const { return VANADIS_REGISTER_MODE_FP32; }

    void setStackPointer( SST::Output* output, VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t start_stack_address ) {
        output->verbose(
            CALL_INFO, 16, 0, "-> Setting SP to (64B-aligned):          %" PRIu64 " / 0x%0llx\n", start_stack_address,
            start_stack_address);
        const int16_t sp_phys_reg = isa_tbl->getIntPhysReg(29);
        output->verbose(CALL_INFO, 16, 0, "-> Stack Pointer (r29) maps to phys-reg: %" PRIu16 "\n", sp_phys_reg);
        // Set up the stack pointer
        // Register 29 is MIPS for Stack Pointer
        regFile->setIntReg(sp_phys_reg, start_stack_address);
    }


    void initStatistics( ) {
        stat_decode_add       = registerStatistic<uint64_t>("ins_decode_add", "1");
        stat_decode_addu      = registerStatistic<uint64_t>("ins_decode_addu", "1");
        stat_decode_and       = registerStatistic<uint64_t>("ins_decode_and", "1");
        stat_decode_dadd      = registerStatistic<uint64_t>("ins_decode_dadd", "1");
        stat_decode_daddu     = registerStatistic<uint64_t>("ins_decode_daddu", "1");
        stat_decode_ddiv      = registerStatistic<uint64_t>("ins_decode_ddiv", "1");
        stat_decode_div       = registerStatistic<uint64_t>("ins_decode_div", "1");
        stat_decode_divu      = registerStatistic<uint64_t>("ins_decode_divu", "1");
        stat_decode_dmult     = registerStatistic<uint64_t>("ins_decode_dmult", "1");
        stat_decode_dmultu    = registerStatistic<uint64_t>("ins_decode_dmultu", "1");
        stat_decode_dsllv     = registerStatistic<uint64_t>("ins_decode_dsllv", "1");
        stat_decode_dsrav     = registerStatistic<uint64_t>("ins_decode_dsrav", "1");
        stat_decode_dsrlv     = registerStatistic<uint64_t>("ins_decode_dsrlv", "1");
        stat_decode_dsub      = registerStatistic<uint64_t>("ins_decode_dsub", "1");
        stat_decode_dsubu     = registerStatistic<uint64_t>("ins_decode_dsubu", "1");
        stat_decode_jr        = registerStatistic<uint64_t>("ins_decode_jr", "1");
        stat_decode_jalr      = registerStatistic<uint64_t>("ins_decode_jalr", "1");
        stat_decode_mfhi      = registerStatistic<uint64_t>("ins_decode_mfhi", "1");
        stat_decode_mflo      = registerStatistic<uint64_t>("ins_decode_mflo", "1");
        stat_decode_mult      = registerStatistic<uint64_t>("ins_decode_mult", "1");
        stat_decode_multu     = registerStatistic<uint64_t>("ins_decode_multu", "1");
        stat_decode_nor       = registerStatistic<uint64_t>("ins_decode_nor", "1");
        stat_decode_or        = registerStatistic<uint64_t>("ins_decode_or", "1");
        stat_decode_sllv      = registerStatistic<uint64_t>("ins_decode_sllv", "1");
        stat_decode_slt       = registerStatistic<uint64_t>("ins_decode_slt", "1");
        stat_decode_sltu      = registerStatistic<uint64_t>("ins_decode_sltu", "1");
        stat_decode_srav      = registerStatistic<uint64_t>("ins_decode_srav", "1");
        stat_decode_srlv      = registerStatistic<uint64_t>("ins_decode_srlv", "1");
        stat_decode_sub       = registerStatistic<uint64_t>("ins_decode_sub", "1");
        stat_decode_subu      = registerStatistic<uint64_t>("ins_decode_subu", "1");
        stat_decode_syscall   = registerStatistic<uint64_t>("ins_decode_syscall", "1");
        stat_decode_sync      = registerStatistic<uint64_t>("ins_decode_sync", "1");
        stat_decode_xor       = registerStatistic<uint64_t>("ins_decode_xor", "1");
        stat_decode_sll       = registerStatistic<uint64_t>("ins_decode_sll", "1");
        stat_decode_srl       = registerStatistic<uint64_t>("ins_decode_srl", "1");
        stat_decode_sra       = registerStatistic<uint64_t>("ins_decode_sra", "1");
        stat_decode_bltz      = registerStatistic<uint64_t>("ins_decode_bltz", "1");
        stat_decode_bgezal    = registerStatistic<uint64_t>("ins_decode_bgezal", "1");
        stat_decode_bgez      = registerStatistic<uint64_t>("ins_decode_bgez", "1");
        stat_decode_lui       = registerStatistic<uint64_t>("ins_decode_lui", "1");
        stat_decode_lb        = registerStatistic<uint64_t>("ins_decode_lb", "1");
        stat_decode_lbu       = registerStatistic<uint64_t>("ins_decode_lbu", "1");
        stat_decode_lhu       = registerStatistic<uint64_t>("ins_decode_lhu", "1");
        stat_decode_lw        = registerStatistic<uint64_t>("ins_decode_lw", "1");
        stat_decode_lfp32     = registerStatistic<uint64_t>("ins_decode_lfp32", "1");
        stat_decode_ll        = registerStatistic<uint64_t>("ins_decode_ll", "1");
        stat_decode_lwl       = registerStatistic<uint64_t>("ins_decode_lwl", "1");
        stat_decode_lwr       = registerStatistic<uint64_t>("ins_decode_lwr", "1");
        stat_decode_sb        = registerStatistic<uint64_t>("ins_decode_sb", "1");
        stat_decode_sc        = registerStatistic<uint64_t>("ins_decode_sc", "1");
        stat_decode_sw        = registerStatistic<uint64_t>("ins_decode_sw", "1");
        stat_decode_sh        = registerStatistic<uint64_t>("ins_decode_sh", "1");
        stat_decode_sfp32     = registerStatistic<uint64_t>("ins_decode_sfp32", "1");
        stat_decode_swr       = registerStatistic<uint64_t>("ins_decode_swr", "1");
        stat_decode_swl       = registerStatistic<uint64_t>("ins_decode_swl", "1");
        stat_decode_addiu     = registerStatistic<uint64_t>("ins_decode_addiu", "1");
        stat_decode_beq       = registerStatistic<uint64_t>("ins_decode_beq", "1");
        stat_decode_bgtz      = registerStatistic<uint64_t>("ins_decode_bgtz", "1");
        stat_decode_blez      = registerStatistic<uint64_t>("ins_decode_blez", "1");
        stat_decode_bne       = registerStatistic<uint64_t>("ins_decode_bne", "1");
        stat_decode_slti      = registerStatistic<uint64_t>("ins_decode_slti", "1");
        stat_decode_sltiu     = registerStatistic<uint64_t>("ins_decode_sltiu", "1");
        stat_decode_andi      = registerStatistic<uint64_t>("ins_decode_andi", "1");
        stat_decode_ori       = registerStatistic<uint64_t>("ins_decode_ori", "1");
        stat_decode_j         = registerStatistic<uint64_t>("ins_decode_j", "1");
        stat_decode_jal       = registerStatistic<uint64_t>("ins_decode_jal", "1");
        stat_decode_xori      = registerStatistic<uint64_t>("ins_decode_xori", "1");
        stat_decode_rdhwr     = registerStatistic<uint64_t>("ins_decode_rdhwr", "1");
        stat_decode_cop1_mtc  = registerStatistic<uint64_t>("ins_decode_cop1_mtc", "1");
        stat_decode_cop1_mfc  = registerStatistic<uint64_t>("ins_decode_cop1_mfc", "1");
        stat_decode_cop1_cf   = registerStatistic<uint64_t>("ins_decode_cop1_cf", "1");
        stat_decode_cop1_ct   = registerStatistic<uint64_t>("ins_decode_cop1_ct", "1");
        stat_decode_cop1_mov  = registerStatistic<uint64_t>("ins_decode_cop1_mov", "1");
        stat_decode_cop1_mul  = registerStatistic<uint64_t>("ins_decode_cop1_mul", "1");
        stat_decode_cop1_div  = registerStatistic<uint64_t>("ins_decode_cop1_div", "1");
        stat_decode_cop1_sub  = registerStatistic<uint64_t>("ins_decode_cop1_sub", "1");
        stat_decode_cop1_cvts = registerStatistic<uint64_t>("ins_decode_cop1_cvts", "1");
        stat_decode_cop1_cvtd = registerStatistic<uint64_t>("ins_decode_cop1_cvtd", "1");
        stat_decode_cop1_cvtw = registerStatistic<uint64_t>("ins_decode_cop1_cvtw", "1");
        stat_decode_cop1_lt   = registerStatistic<uint64_t>("ins_decode_cop1_lt", "1");
        stat_decode_cop1_lte  = registerStatistic<uint64_t>("ins_decode_cop1_lte", "1");
        stat_decode_cop1_eq   = registerStatistic<uint64_t>("ins_decode_cop1_eq", "1");
    }

    virtual void tick(SST::Output* output, uint64_t cycle)
    {
        output->verbose(CALL_INFO, 16, 0, "-> Decode step for thr: %" PRIu32 "\n", hw_thr);
        output->verbose(CALL_INFO, 16, 0, "---> Max decodes per cycle: %" PRIu16 "\n", max_decodes_per_cycle);

        ins_loader->printStatus(output);

        uint16_t decodes_performed = 0;
        uint16_t uop_bundles_used  = 0;

        for ( uint16_t i = 0; i < max_decodes_per_cycle; ++i ) {
            // if the ROB has space, then lets go ahead and
            // decode the input, put it in the queue for issue.
            if ( !thread_rob->full() ) {
                if ( ins_loader->hasBundleAt(ip) ) {
                    output->verbose(
                        CALL_INFO, 16, 0, "---> Found uop bundle for ip=0x0%llx, loading from cache...\n", ip);
                    VanadisInstructionBundle* bundle = ins_loader->getBundleAt(ip);
                    stat_uop_hit->addData(1);

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> Bundle contains %" PRIu32 " entries.\n",
                        bundle->getInstructionCount());

                    if ( 0 == bundle->getInstructionCount() ) {
                        output->fatal(CALL_INFO, -1, "------> STOP - bundle at 0x%0llx contains zero entries.\n", ip);
                    }

                    bool q_contains_store = false;

                    output->verbose(
                        CALL_INFO, 16, 0, "----> thr-rob contains %" PRIu32 " entries.\n",
                        (uint32_t)thread_rob->size());

                    // Check if last instruction is a BRANCH, if yes, we need to also
                    // decode the branch-delay slot AND handle the prediction
                    if ( bundle->getInstructionByIndex(bundle->getInstructionCount() - 1)->getInstFuncType() ==
                         INST_BRANCH ) {
                        output->verbose(
                            CALL_INFO, 16, 0,
                            "-----> Last instruction in the bundle causes potential "
                            "branch, checking on branch delay slot\n");

                        VanadisInstructionBundle* delay_bundle = nullptr;
                        uint32_t                  temp_delay   = 0;

                        if ( ins_loader->hasBundleAt(ip + 4) ) {
                            // We have also decoded the branch-delay
                            delay_bundle = ins_loader->getBundleAt(ip + 4);
                            stat_uop_hit->addData(1);
                        }
                        else {
                            output->verbose(
                                CALL_INFO, 16, 0,
                                "-----> Branch delay slot is not currently "
                                "decoded into a bundle.\n");
                            if ( ins_loader->hasPredecodeAt(ip + 4, 4) ) {
                                output->verbose(
                                    CALL_INFO, 16, 0,
                                    "-----> Branch delay slot is a pre-decode "
                                    "cache item, decode it and keep bundle.\n");
                                delay_bundle = new VanadisInstructionBundle(ip + 4);

                                if ( ins_loader->getPredecodeBytes(
                                         output, ip + 4, (uint8_t*)&temp_delay, sizeof(temp_delay)) ) {
                                    stat_predecode_hit->addData(1);

                                    decode(output, ip + 4, temp_delay, delay_bundle);
                                    ins_loader->cacheDecodedBundle(delay_bundle);
                                    decodes_performed++;
                                }
                                else {
                                    output->fatal(
                                        CALL_INFO, -1,
                                        "Error: instruction loader has bytes for delay slot at "
                                        "%0llx, but they cannot be retrieved.\n",
                                        (ip + 4));
                                }
                            }
                            else {
                                output->verbose(
                                    CALL_INFO, 16, 0,
                                    "-----> Branch delay slot also misses in "
                                    "pre-decode cache, need to request it.\n");
                                ins_loader->requestLoadAt(output, ip + 4, 4);
                                stat_ins_bytes_loaded->addData(4);
                                stat_predecode_miss->addData(1);
                            }
                        }

                        // We have the branch AND the delay, now lets issue them.
                        if ( nullptr != delay_bundle ) {
                            if ( (bundle->getInstructionCount() + delay_bundle->getInstructionCount()) <
                                 (thread_rob->capacity() - thread_rob->size()) ) {

                                output->verbose(
                                    CALL_INFO, 16, 0,
                                    "---> Proceeding with issue the branch and its "
                                    "delay slot...\n");

                                for ( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
                                    VanadisInstruction* next_ins = bundle->getInstructionByIndex(i)->clone();

                                    output->verbose(
                                        CALL_INFO, 16, 0, "---> --> issuing ins addr: 0x0%llx, %s...\n",
                                        next_ins->getInstructionAddress(), next_ins->getInstCode());

                                    thread_rob->push(next_ins);

                                    // if this is the last instruction in the bundle
                                    // we need to obtain a branch prediction
                                    if ( i == (bundle->getInstructionCount() - 1) ) {
                                        VanadisSpeculatedInstruction* speculated_ins =
                                            dynamic_cast<VanadisSpeculatedInstruction*>(next_ins);

                                        // Do we have an entry for the branch instruction we just
                                        // issued
                                        if ( branch_predictor->contains(ip) ) {
                                            const uint64_t predicted_address = branch_predictor->predictAddress(ip);
                                            speculated_ins->setSpeculatedAddress(predicted_address);

                                            // This is essential a predicted not taken branch
                                            if ( predicted_address == (ip + 8) ) {
                                                output->verbose(
                                                    CALL_INFO, 16, 0,
                                                    "---> Branch 0x%llx predicted not "
                                                    "taken, ip set to: 0x%0llx\n",
                                                    ip, predicted_address);
                                                //												speculated_ins->setSpeculatedDirection(
                                                // BRANCH_NOT_TAKEN );
                                            }
                                            else {
                                                output->verbose(
                                                    CALL_INFO, 16, 0,
                                                    "---> Branch 0x%llx predicted taken, "
                                                    "jump to 0x%0llx\n",
                                                    ip, predicted_address);
                                                //												speculated_ins->setSpeculatedDirection(
                                                // BRANCH_TAKEN );
                                            }

                                            ip = predicted_address;
                                            output->verbose(
                                                CALL_INFO, 16, 0,
                                                "---> Forcing IP update according to branch "
                                                "prediction table, new-ip: %0llx\n",
                                                ip);
                                        }
                                        else {
                                            output->verbose(
                                                CALL_INFO, 16, 0,
                                                "---> Branch table does not contain an "
                                                "entry for ins: 0x%0llx, continue with "
                                                "normal ip += 8 = 0x%0llx\n",
                                                ip, (ip + 8));

                                            //											speculated_ins->setSpeculatedDirection(
                                            // BRANCH_NOT_TAKEN );
                                            speculated_ins->setSpeculatedAddress(ip + 8);

                                            // We don't urgh.. let's just carry on
                                            // remember we increment the IP by 2 instructions (me +
                                            // delay)
                                            ip += 8;
                                        }
                                    }
                                }

                                for ( uint32_t i = 0; i < delay_bundle->getInstructionCount(); ++i ) {
                                    VanadisInstruction* next_ins = delay_bundle->getInstructionByIndex(i)->clone();

                                    output->verbose(
                                        CALL_INFO, 16, 0, "---> --> issuing ins addr: 0x0%llx, %s...\n",
                                        next_ins->getInstructionAddress(), next_ins->getInstCode());
                                    thread_rob->push(next_ins);
                                }

                                uop_bundles_used += 2;
                            }
                            else {
                                output->verbose(
                                    CALL_INFO, 16, 0,
                                    "---> --> micro-op for branch and delay exceed "
                                    "decode-q space. Cannot issue this cycle.\n");
                                break;
                            }
                        }
                    }
                    else {
                        output->verbose(
                            CALL_INFO, 16, 0,
                            "---> Instruction for issue is not a branch, "
                            "continuing with normal copy to issue-queue...\n");
                        // Do we have enough space in the decode queue for the bundle
                        // contents?
                        if ( bundle->getInstructionCount() < (thread_rob->capacity() - thread_rob->size()) ) {
                            // Put in the queue
                            for ( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
                                VanadisInstruction* next_ins = bundle->getInstructionByIndex(i);

                                output->verbose(
                                    CALL_INFO, 16, 0, "---> --> issuing ins addr: 0x0%llx, %s...\n",
                                    next_ins->getInstructionAddress(), next_ins->getInstCode());
                                thread_rob->push(next_ins->clone());
                            }

                            uop_bundles_used++;

                            // Push the instruction pointer along by the standard amount
                            ip += 4;
                        }
                        else {
                            output->verbose(
                                CALL_INFO, 16, 0,
                                "---> --> micro-op bundle for %p contains %" PRIu32 " ops, we only have %" PRIu32
                                " slots available in the decode q, wait for resources to "
                                "become available.\n",
                                (void*)ip, (uint32_t)bundle->getInstructionCount(),
                                (uint32_t)(thread_rob->capacity() - thread_rob->size()));
                            // We don't have enough space, so we have to stop and wait for
                            // more entries to free up.
                            break;
                        }
                    }
                }
                else if ( ins_loader->hasPredecodeAt(ip, 4) ) {
                    // We do have a locally cached copy of the data at the IP though, so
                    // decode into a bundle
                    output->verbose(
                        CALL_INFO, 16, 0,
                        "---> uop not found, but matched in predecoded "
                        "L0-icache (ip=%p)\n",
                        (void*)ip);
                    stat_predecode_hit->addData(1);

                    uint32_t                  temp_ins       = 0;
                    VanadisInstructionBundle* decoded_bundle = new VanadisInstructionBundle(ip);

                    if ( ins_loader->getPredecodeBytes(output, ip, (uint8_t*)&temp_ins, sizeof(temp_ins)) ) {
                        output->verbose(
                            CALL_INFO, 16, 0,
                            "---> performing a decode of the bytes found "
                            "(ins-bytes: 0x%x)\n",
                            temp_ins);
                        decode(output, ip, temp_ins, decoded_bundle);

                        output->verbose(
                            CALL_INFO, 16, 0,
                            "---> performing a decode of the bytes found "
                            "(generates %" PRIu32 " micro-op bundle).\n",
                            (uint32_t)decoded_bundle->getInstructionCount());
                        ins_loader->cacheDecodedBundle(decoded_bundle);
                        decodes_performed++;

                        break;
                    }
                    else {
                        output->fatal(
                            CALL_INFO, -1,
                            "Error: predecoded bytes found at ip=%p, but %d byte "
                            "retrival failed.\n",
                            (void*)ip, (int)sizeof(temp_ins));
                    }
                }
                else {
                    output->verbose(
                        CALL_INFO, 16, 0,
                        "---> uop bundle and pre-decoded bytes are not found "
                        "(ip=%p), requesting icache read (line-width=%" PRIu64 ")\n",
                        (void*)ip, ins_loader->getCacheLineWidth());
                    ins_loader->requestLoadAt(output, ip, 4);
                    stat_ins_bytes_loaded->addData(4);
                    stat_predecode_miss->addData(1);
                    break;
                }
            }
            else {
                output->verbose(
                    CALL_INFO, 16, 0,
                    "---> Decoded pending issue queue is full, no more "
                    "decodes permitted.\n");
                break;
            }
        }

        output->verbose(
            CALL_INFO, 16, 0,
            "---> Performed %" PRIu16 " decodes this cycle, %" PRIu16 " uop-bundles used / updated-ip: 0x%llx.\n",
            decodes_performed, uop_bundles_used, ip);
    }

protected:
    void extract_imm(const uint32_t ins, uint32_t* imm) const { (*imm) = (ins & MIPS_IMM_MASK); }

    void extract_three_regs(const uint32_t ins, uint16_t* rt, uint16_t* rs, uint16_t* rd) const
    {
        (*rt) = (ins & MIPS_RT_MASK) >> 16;
        (*rs) = (ins & MIPS_RS_MASK) >> 21;
        (*rd) = (ins & MIPS_RD_MASK) >> 11;
    }

    void extract_fp_regs(const uint32_t ins, uint16_t* fr, uint16_t* ft, uint16_t* fs, uint16_t* fd) const
    {
        (*fr) = (ins & MIPS_FR_MASK) >> 21;
        (*ft) = (ins & MIPS_FT_MASK) >> 16;
        (*fs) = (ins & MIPS_FS_MASK) >> 11;
        (*fd) = (ins & MIPS_FD_MASK) >> 6;
    }

    void decode(SST::Output* output, const uint64_t ins_addr, const uint32_t next_ins, VanadisInstructionBundle* bundle)
    {
        output->verbose(CALL_INFO, 16, 0, "[decode] > addr: 0x%llx ins: 0x%08x\n", ins_addr, next_ins);

        const uint32_t hw_thr    = getHardwareThread();
        const uint32_t ins_mask  = next_ins & MIPS_OP_MASK;
        const uint32_t func_mask = next_ins & MIPS_FUNC_MASK;

        if ( 0 != (ins_addr & 0x3) ) {
            output->verbose(
                CALL_INFO, 16, 0, "[decode] ---> fault address 0x%llu is not aligned at 4 bytes.\n", ins_addr);
            bundle->addInstruction(new VanadisInstructionDecodeFault(ins_addr, hw_thr, options));
            return;
        }

        if ( haltOnDecodeZero && (0 == ins_addr) ) {
            bundle->addInstruction(new VanadisInstructionDecodeFault(ins_addr, getHardwareThread(), options));
            return;
        }

        output->verbose(CALL_INFO, 16, 0, "[decode] ---> ins-mask: 0x%08x / 0x%08x\n", ins_mask, func_mask);

        uint16_t rt = 0;
        uint16_t rs = 0;
        uint16_t rd = 0;

        uint32_t imm = 0;

        // Perform a register and parameter extract in case we need later
        extract_three_regs(next_ins, &rt, &rs, &rd);
        extract_imm(next_ins, &imm);

        output->verbose(CALL_INFO, 16, 0, "[decode] rt=%" PRIu32 ", rs=%" PRIu32 ", rd=%" PRIu32 "\n", rt, rs, rd);

        const uint64_t imm64 = (uint64_t)imm;

        bool insertDecodeFault = true;

        // Check if this is a NOP, this is fairly frequent due to use in delay
        // slots, do not spend time decoding this
        if ( 0 == next_ins ) {
            bundle->addInstruction(new VanadisNoOpInstruction(ins_addr, hw_thr, options));
            insertDecodeFault = false;
        }
        else {

            output->verbose(CALL_INFO, 16, 0, "[decode] -> inst-mask: 0x%08x\n", ins_mask);

            switch ( ins_mask ) {
            case 0:
            {
                // The SHIFT 5 bits must be zero for these operations according to the
                // manual
                if ( 0 == (next_ins & MIPS_SHFT_MASK) ) {
                    output->verbose(CALL_INFO, 16, 0, "[decode] -> special-class, func-mask: 0x%x\n", func_mask);

                    if ( (0 == func_mask) && (0 == rs) ) {
                        output->verbose(
                            CALL_INFO, 16, 0,
                            "[decode] -> rs is also zero, implies truncate "
                            "(generate: 64 to 32 truncate)\n");
                        bundle->addInstruction(
                            new VanadisTruncateInstruction<
                                VanadisRegisterFormat::VANADIS_FORMAT_INT64,
                                VanadisRegisterFormat::VANADIS_FORMAT_INT32>(ins_addr, hw_thr, options, rd, rt));
                        insertDecodeFault = false;
                    }
                    else {
                        switch ( func_mask ) {
                        case MIPS_SPEC_OP_MASK_ADD:
                        {
                            bundle->addInstruction(
                                new VanadisAddInstruction<int32_t>(
                                    ins_addr, hw_thr, options, rd, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_add);
                        } break;

                        case MIPS_SPEC_OP_MASK_ADDU:
                        {
                            bundle->addInstruction(
                                new VanadisAddInstruction<int32_t>(
                                    ins_addr, hw_thr, options, rd, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_addu);
                        } break;

                        case MIPS_SPEC_OP_MASK_AND:
                        {
                            bundle->addInstruction(new VanadisAndInstruction(ins_addr, hw_thr, options, rd, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_and);
                        } break;

                            // _BREAK NEEDS TO GO HERE?

                        case MIPS_SPEC_OP_MASK_DADD:
                            break;

                        case MIPS_SPEC_OP_MASK_DADDU:
                            break;

                        case MIPS_SPEC_OP_MASK_DDIV:
                            break;

                        case MIPS_SPEC_OP_MASK_DDIVU:
                            break;

                        case MIPS_SPEC_OP_MASK_DIV:
                        {
                            bundle->addInstruction(new VanadisDivideRemainderInstruction<int32_t>(
                                ins_addr, hw_thr, options, MIPS_REG_LO, MIPS_REG_HI, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_div);
                        } break;

                        case MIPS_SPEC_OP_MASK_DIVU:
                        {
                            bundle->addInstruction(new VanadisDivideRemainderInstruction<uint32_t>(
                                ins_addr, hw_thr, options, MIPS_REG_LO, MIPS_REG_HI, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_divu);
                        } break;

                        case MIPS_SPEC_OP_MASK_DMULT:
                            break;

                        case MIPS_SPEC_OP_MASK_DMULTU:
                            break;

                        case MIPS_SPEC_OP_MASK_DSLLV:
                            break;

                        case MIPS_SPEC_OP_MASK_DSRAV:
                            break;

                        case MIPS_SPEC_OP_MASK_DSRLV:
                            break;

                        case MIPS_SPEC_OP_MASK_DSUB:
                            break;

                        case MIPS_SPEC_OP_MASK_DSUBU:
                            break;

                        case MIPS_SPEC_OP_MASK_JR:
                        {

                            bundle->addInstruction(new VanadisJumpRegInstruction(
                                ins_addr, hw_thr, options, 4, rs, VANADIS_SINGLE_DELAY_SLOT));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_jr);
                        } break;

                        case MIPS_SPEC_OP_MASK_JALR:
                        {
                            bundle->addInstruction(new VanadisJumpRegLinkInstruction(
                                ins_addr, hw_thr, options, 4, rd, rs, 0, VANADIS_SINGLE_DELAY_SLOT));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_jalr);
                        } break;

                        case MIPS_SPEC_OP_MASK_MFHI:
                        {
                            // Special instruction_, 32 is LO, 33 is HI
                            bundle->addInstruction(
                                new VanadisAddImmInstruction<int32_t>(
                                    ins_addr, hw_thr, options, rd, MIPS_REG_HI, 0));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_mfhi);
                        } break;

                        case MIPS_SPEC_OP_MASK_MFLO:
                        {
                            // Special instruction, 32 is LO, 33 is HI
                            bundle->addInstruction(
                                new VanadisAddImmInstruction<int32_t>(
                                    ins_addr, hw_thr, options, rd, MIPS_REG_LO, 0));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_mflo);
                        } break;

                        case MIPS_SPEC_OP_MASK_MOVN:
                        {
                            bundle->addInstruction(new VanadisMoveCompareImmInstruction<int32_t>(
                                ins_addr, hw_thr, options, rd, rs, rt, 0, REG_COMPARE_NEQ));
                            insertDecodeFault = false;
                        } break;

                        case MIPS_SPEC_OP_MASK_MOVZ:
                        {
                            bundle->addInstruction(new VanadisMoveCompareImmInstruction<int32_t>(
                                ins_addr, hw_thr, options, rd, rs, rt, 0, REG_COMPARE_EQ));
                            insertDecodeFault = false;
                        } break;

                        case MIPS_SPEC_OP_MASK_MTHI:
                            break;

                        case MIPS_SPEC_OP_MASK_MTLO:
                            break;

                        case MIPS_SPEC_OP_MASK_MULT:
                        {
                            bundle->addInstruction(
                                new VanadisMultiplySplitInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32, true>(
                                    ins_addr, hw_thr, options, MIPS_REG_LO, MIPS_REG_HI, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_mult);
                        } break;

                        case MIPS_SPEC_OP_MASK_MULTU:
                        {
                            bundle->addInstruction(
                                new VanadisMultiplySplitInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32, false>(
                                    ins_addr, hw_thr, options, MIPS_REG_LO, MIPS_REG_HI, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_multu);
                            ;
                        } break;

                        case MIPS_SPEC_OP_MASK_NOR:
                        {
                            bundle->addInstruction(new VanadisNorInstruction(ins_addr, hw_thr, options, rd, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_nor);
                        } break;

                        case MIPS_SPEC_OP_MASK_OR:
                        {
                            bundle->addInstruction(new VanadisOrInstruction(ins_addr, hw_thr, options, rd, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_or);
                        } break;

                        case MIPS_SPEC_OP_MASK_SLLV:
                        {
                            bundle->addInstruction(
                                new VanadisShiftLeftLogicalInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                    ins_addr, hw_thr, options, rd, rt, rs));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_sllv);
                        } break;

                        case MIPS_SPEC_OP_MASK_SLT:
                        {
                            bundle->addInstruction(new VanadisSetRegCompareInstruction<
                                                   REG_COMPARE_LT, VanadisRegisterFormat::VANADIS_FORMAT_INT32, true>(
                                ins_addr, hw_thr, options, rd, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_slt);
                        } break;

                        case MIPS_SPEC_OP_MASK_SLTU:
                        {
                            bundle->addInstruction(new VanadisSetRegCompareInstruction<
                                                   REG_COMPARE_LT, VanadisRegisterFormat::VANADIS_FORMAT_INT32, false>(
                                ins_addr, hw_thr, options, rd, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_sltu);
                        } break;

                        case MIPS_SPEC_OP_MASK_SRAV:
                        {
                            bundle->addInstruction(
                                new VanadisShiftRightArithmeticInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                    ins_addr, hw_thr, options, rd, rt, rs));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_srav);
                        } break;

                        case MIPS_SPEC_OP_MASK_SRLV:
                        {
                            bundle->addInstruction(
                                new VanadisShiftRightLogicalInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                    ins_addr, hw_thr, options, rd, rt, rs));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_srlv);
                        } break;

                        case MIPS_SPEC_OP_MASK_SUB:
                        {
                            bundle->addInstruction(
                                new VanadisSubInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                    ins_addr, hw_thr, options, rd, rs, rt, true));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_sub);
                        } break;

                        case MIPS_SPEC_OP_MASK_SUBU:
                        {
                            bundle->addInstruction(
                                new VanadisSubInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                    ins_addr, hw_thr, options, rd, rs, rt, false));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_subu);
                        } break;

                        case MIPS_SPEC_OP_MASK_SYSCALL:
                        {
                            bundle->addInstruction(new VanadisFenceInstruction(ins_addr, hw_thr, options, VANADIS_LOAD_STORE_FENCE));
                            bundle->addInstruction(new VanadisSysCallInstruction(ins_addr, hw_thr, options));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_syscall);
                        } break;

                        case MIPS_SPEC_OP_MASK_SYNC:
                        {
                            bundle->addInstruction(
                                new VanadisFenceInstruction(ins_addr, hw_thr, options, VANADIS_LOAD_STORE_FENCE));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_sync);
                        } break;

                        case MIPS_SPEC_OP_MASK_XOR:
                            bundle->addInstruction(new VanadisXorInstruction(ins_addr, hw_thr, options, rd, rs, rt));
                            insertDecodeFault = false;
                            MIPS_INC_DECODE_STAT(stat_decode_xor);
                            break;
                        }
                    }
                }
                else {
                    switch ( func_mask ) {
                    case MIPS_SPEC_OP_MASK_SLL:
                    {
                        const uint64_t shf_amnt = ((uint64_t)(next_ins & MIPS_SHFT_MASK)) >> 6;

                        output->verbose(
                            CALL_INFO, 16, 0, "[decode/SLL]-> out: %" PRIu16 " / in: %" PRIu16 " shft: %" PRIu64 "\n",
                            rd, rt, shf_amnt);

                        bundle->addInstruction(
                            new VanadisShiftLeftLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                ins_addr, hw_thr, options, rd, rt, shf_amnt));
                        insertDecodeFault = false;
                        MIPS_INC_DECODE_STAT(stat_decode_sll);
                    } break;

                    case MIPS_SPEC_OP_MASK_SRL:
                    {
                        const uint64_t shf_amnt = ((uint64_t)(next_ins & MIPS_SHFT_MASK)) >> 6;

                        output->verbose(
                            CALL_INFO, 16, 0, "[decode/SRL]-> out: %" PRIu16 " / in: %" PRIu16 " shft: %" PRIu64 "\n",
                            rd, rt, shf_amnt);

                        bundle->addInstruction(
                            new VanadisShiftRightLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                ins_addr, hw_thr, options, rd, rt, shf_amnt));
                        insertDecodeFault = false;
                        MIPS_INC_DECODE_STAT(stat_decode_srl);
                    } break;

                    case MIPS_SPEC_OP_MASK_SRA:
                    {
                        const uint64_t shf_amnt = ((uint64_t)(next_ins & MIPS_SHFT_MASK)) >> 6;

                        bundle->addInstruction(
                            new VanadisShiftRightArithmeticImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                ins_addr, hw_thr, options, rd, rt, shf_amnt));
                        insertDecodeFault = false;
                        MIPS_INC_DECODE_STAT(stat_decode_sra);
                    } break;
                    }
                }
            } break;

            case MIPS_SPEC_OP_MASK_LW:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/LW]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisLoadInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 4, true, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_lw);
            } break;

            case MIPS_SPEC_OP_MASK_SW:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/SW]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisStoreInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 4, MEM_TRANSACTION_NONE, STORE_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_sw);
            } break;

            case MIPS_SPEC_OP_MASK_LB:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //                                output->verbose(CALL_INFO, 16, 0,
                //                                "[decoder/LB]: -> reg: %" PRIu16 " <-
                //                                base: %" PRIu16 " + offset=%" PRId64
                //                                "\n",
                //                                        rt, rs, imm_value_64);

                bundle->addInstruction(new VanadisLoadInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 1, true, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
                insertDecodeFault = false;
            } break;

            case MIPS_SPEC_OP_MASK_LBU:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //                                output->verbose(CALL_INFO, 16, 0,
                //                                "[decoder/LBU]: -> reg: %" PRIu16 " <-
                //                                base: %" PRIu16 " + offset=%" PRId64
                //                                "\n",
                //                                        rt, rs, imm_value_64);

                bundle->addInstruction(new VanadisLoadInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 1, false, MEM_TRANSACTION_NONE,
                    LOAD_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_lbu);
            } break;

            case MIPS_SPEC_OP_MASK_LFP32:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/LFP32]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisLoadInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 4, true, MEM_TRANSACTION_NONE, LOAD_FP_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_lfp32);
            } break;

            case MIPS_SPEC_OP_MASK_LL:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/LL]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisLoadInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 4, true, MEM_TRANSACTION_LLSC_LOAD,
                    LOAD_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_ll);
            } break;

            case MIPS_SPEC_OP_MASK_LWL:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/LWL (PARTLOAD)]: -> reg: %" PRIu16 " <- base: %" PRIu16 " +
                // offset=%" PRId64 "\n",
                //                                        rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisPartialLoadInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 4, true, false, LOAD_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_lwl);
            } break;

            case MIPS_SPEC_OP_MASK_LWR:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/LWR (PARTLOAD)]: -> reg: %" PRIu16 " <- base: %" PRIu16 " +
                // offset=%" PRId64 "\n",
                //                                        rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisPartialLoadInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 4, true, true, LOAD_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_lwr);
            } break;

            case MIPS_SPEC_OP_MASK_LHU:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/LHU]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisLoadInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 2, false, MEM_TRANSACTION_NONE,
                    LOAD_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_lhu);
            } break;

            case MIPS_SPEC_OP_MASK_SB:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/SB]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisStoreInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 1, MEM_TRANSACTION_NONE, STORE_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_sb);
            } break;

            case MIPS_SPEC_OP_MASK_SH:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/SH]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisStoreInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 2, MEM_TRANSACTION_NONE, STORE_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_sh);
            } break;

            case MIPS_SPEC_OP_MASK_SFP32:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/SFP32]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisStoreInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 4, MEM_TRANSACTION_NONE, STORE_FP_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_sfp32);
            } break;

            case MIPS_SPEC_OP_MASK_SWL:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/SWL]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisPartialStoreInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 4, true, STORE_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_swl);
            } break;

            case MIPS_SPEC_OP_MASK_SWR:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/SWR]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisPartialStoreInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 4, false, STORE_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_swr);
            } break;

            case MIPS_SPEC_OP_MASK_SC:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/SC]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%"
                // PRId64 "\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisStoreConditionalInstruction(
                    ins_addr, hw_thr, options, rs, imm_value_64, rt, rt, 4, STORE_INT_REGISTER));
                //                bundle->addInstruction(new VanadisStoreInstruction(
                //                    ins_addr, hw_thr, options, rs, imm_value_64, rt, 4, MEM_TRANSACTION_LLSC_STORE,
                //                    STORE_INT_REGISTER));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_sc);
            } break;

            case MIPS_SPEC_OP_MASK_REGIMM:
            {
                const uint64_t offset_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);
                ;

#ifdef VANADIS_BUILD_DEBUG
                output->verbose(CALL_INFO, 16, 0, "[decoder/REGIMM] -> imm: %" PRIu64 "\n", offset_value_64);
                output->verbose(CALL_INFO, 16, 0, "[decoder]        -> rt: 0x%08x\n", (next_ins & MIPS_RT_MASK));
#endif

                switch ( (next_ins & MIPS_RT_MASK) ) {
                case MIPS_SPEC_OP_MASK_BLTZ:
                {
                    bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<
                                           int32_t, REG_COMPARE_LT>(
                        ins_addr, hw_thr, options, 4, rs, 0, offset_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
                    insertDecodeFault = false;
                    MIPS_INC_DECODE_STAT(stat_decode_bltz);
                } break;
                case MIPS_SPEC_OP_MASK_BGEZAL:
                {
                    bundle->addInstruction(new VanadisBranchRegCompareImmLinkInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT32, REG_COMPARE_GTE>(
                        ins_addr, hw_thr, options, 4, rs, 0, offset_value_64 + 4, (uint16_t)31,
                        VANADIS_SINGLE_DELAY_SLOT));
                    insertDecodeFault = false;
                    MIPS_INC_DECODE_STAT(stat_decode_bgezal);
                } break;
                case MIPS_SPEC_OP_MASK_BGEZ:
                {
                    bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<
                                           int32_t, REG_COMPARE_GTE>(
                        ins_addr, hw_thr, options, 4, rs, 0, offset_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
                    insertDecodeFault = false;
                    MIPS_INC_DECODE_STAT(stat_decode_bgez);
                } break;
                }

            } break;

            case MIPS_SPEC_OP_MASK_LUI:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 16);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/LUI] -> reg: %" PRIu16 " / imm=%" PRId64 "\n", 					rt,
                // imm_value_64);
                bundle->addInstruction(new VanadisSetRegisterInstruction<int32_t>(
                    ins_addr, hw_thr, options, rt, imm_value_64));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_lui);
            } break;

            case MIPS_SPEC_OP_MASK_ADDIU:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);
                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/ADDIU]: -> reg: %" PRIu16 " rs=%" PRIu16 " / imm=%" PRId64
                //"\n", 					rt, rs, imm_value_64);
                bundle->addInstruction(new VanadisAddImmInstruction<int32_t>(
                    ins_addr, hw_thr, options, rt, rs, imm_value_64));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_addiu);
            } break;

            case MIPS_SPEC_OP_MASK_BEQ:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/BEQ]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64
                //"\n",
                //                                        rt, rs, imm_value_64 );
                bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                       VanadisRegisterFormat::VANADIS_FORMAT_INT32, REG_COMPARE_EQ, true>(
                    ins_addr, hw_thr, options, 4, rt, rs, imm_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_beq);
            } break;

            case MIPS_SPEC_OP_MASK_BGTZ:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/BGTZ]: -> r1: %" PRIu16 " offset: %" PRId64 "\n",
                //                                        rs, imm_value_64);
                bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<
                                       int32_t, REG_COMPARE_GT>(
                    ins_addr, hw_thr, options, 4, rs, 0, imm_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_bgtz);
            } break;

            case MIPS_SPEC_OP_MASK_BLEZ:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/BLEZ]: -> r1: %" PRIu16 " offset: %" PRId64 "\n",
                //                                        rs, imm_value_64);
                bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<
                                       int32_t, REG_COMPARE_LTE>(
                    ins_addr, hw_thr, options, 4, rs, 0, imm_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_blez);
            } break;

            case MIPS_SPEC_OP_MASK_BNE:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/BNE]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64
                //"\n",
                //                                        rt, rs, imm_value_64 );
                bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                       VanadisRegisterFormat::VANADIS_FORMAT_INT32, REG_COMPARE_NEQ, true>(
                    ins_addr, hw_thr, options, 4, rt, rs, imm_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_bne);
            } break;

            case MIPS_SPEC_OP_MASK_SLTI:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/SLTI]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64
                //"\n",
                //                                        rt, rs, imm_value_64 );
                bundle->addInstruction(new VanadisSetRegCompareImmInstruction<
                                       REG_COMPARE_LT, VanadisRegisterFormat::VANADIS_FORMAT_INT32, true>(
                    ins_addr, hw_thr, options, rt, rs, imm_value_64));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_slti);
            } break;

            case MIPS_SPEC_OP_MASK_SLTIU:
            {
                const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/SLTIU]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64
                //"\n",
                //                                        rt, rs, imm_value_64 );
                bundle->addInstruction(new VanadisSetRegCompareImmInstruction<
                                       REG_COMPARE_LT, VanadisRegisterFormat::VANADIS_FORMAT_INT32, false>(
                    ins_addr, hw_thr, options, rt, rs, imm_value_64));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_sltiu);
            } break;

            case MIPS_SPEC_OP_MASK_ANDI:
            {
                // note - ANDI is zero extended, not sign extended
                const uint64_t imm_value_64 = static_cast<uint64_t>(next_ins & MIPS_IMM_MASK);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/ANDI]: -> %" PRIu16 " <- r2: %" PRIu16 " imm: %" PRIu64
                //"\n",
                //                                        rt, rs, imm_value_64 );
                bundle->addInstruction(new VanadisAndImmInstruction(ins_addr, hw_thr, options, rt, rs, imm_value_64));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_andi);
            } break;

            case MIPS_SPEC_OP_MASK_ORI:
            {
                const uint64_t imm_value_64 = static_cast<uint64_t>(next_ins & MIPS_IMM_MASK);

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/ORI]: -> %" PRIu16 " <- r2: %" PRIu16 " imm: %" PRId64 "\n",
                //                                        rt, rs, imm_value_64 );
                bundle->addInstruction(new VanadisOrImmInstruction(ins_addr, hw_thr, options, rt, rs, imm_value_64));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_ori);
            } break;

            case MIPS_SPEC_OP_MASK_J:
            {
                const uint32_t j_addr_index = (next_ins & MIPS_J_ADDR_MASK) << 2;
                const uint32_t upper_bits   = ((ins_addr + 4) & MIPS_J_UPPER_MASK);

                uint64_t jump_to = 0;
                jump_to += (uint64_t)j_addr_index;
                jump_to |= (uint64_t)upper_bits;

                //				output->verbose(CALL_INFO, 16, 0,
                //"[decoder/J]: -> jump-to: %" PRIu64 " / 0x%0llx\n", 					jump_to,
                // jump_to);

                bundle->addInstruction(
                    new VanadisJumpInstruction(ins_addr, hw_thr, options, 4, jump_to, VANADIS_SINGLE_DELAY_SLOT));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_j);
            } break;
            case MIPS_SPEC_OP_MASK_JAL:
            {
                const uint32_t j_addr_index = (next_ins & MIPS_J_ADDR_MASK) << 2;
                const uint32_t upper_bits   = ((ins_addr + 4) & MIPS_J_UPPER_MASK);

                uint64_t jump_to = 0;
                jump_to          = jump_to + (uint64_t)j_addr_index;
                jump_to          = jump_to + (uint64_t)upper_bits;

                //				bundle->addInstruction( new
                // VanadisSetRegisterInstruction( ins_addr, hw_thr, options, 31, ins_addr
                //+ 8 ) );
                bundle->addInstruction(new VanadisJumpLinkInstruction(
                    ins_addr, hw_thr, options, 4, 31, jump_to, VANADIS_SINGLE_DELAY_SLOT));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_jal);
            } break;

            case MIPS_SPEC_OP_MASK_XORI:
            {
                const uint64_t xor_mask = static_cast<uint64_t>(next_ins & MIPS_IMM_MASK);

                bundle->addInstruction(new VanadisXorImmInstruction(ins_addr, hw_thr, options, rt, rs, xor_mask));
                insertDecodeFault = false;
                MIPS_INC_DECODE_STAT(stat_decode_xori);
            }

            case MIPS_SPEC_OP_MASK_COP1:
            {
                //				output->verbose(CALL_INFO, 16, 0, "[decode]
                //--> reached co-processor function decoder\n");

                uint16_t fr = 0;
                uint16_t ft = 0;
                uint16_t fs = 0;
                uint16_t fd = 0;

                extract_fp_regs(next_ins, &fr, &ft, &fs, &fd);

                if ( (next_ins & 0x3E30000) == 0x1010000 ) {
                    // this decodes to a BRANCH on TRUE
                    const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

                    bundle->addInstruction(new VanadisBranchFPInstruction(
                        ins_addr, hw_thr, options, 4, MIPS_FP_STATUS_REG, imm_value_64 + 4,
                        /* branch on true */ true, VANADIS_SINGLE_DELAY_SLOT));
                    insertDecodeFault = false;
                }
                else if ( (next_ins & 0x3E30000) == 0x1000000 ) {
                    // this decodes to a BRANCH on FALSE
                    const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

                    bundle->addInstruction(new VanadisBranchFPInstruction(
                        ins_addr, hw_thr, options, 4, MIPS_FP_STATUS_REG, imm_value_64 + 4,
                        /* branch on false */ false, VANADIS_SINGLE_DELAY_SLOT));
                    insertDecodeFault = false;
                }
                else {

                    //				output->verbose(CALL_INFO, 16, 0, "[decoder] ---->
                    // decoding function mask: %" PRIu32 " / 0x%x\n", 					(next_ins &
                    // MIPS_FUNC_MASK), (next_ins & MIPS_FUNC_MASK) );

                    switch ( next_ins & MIPS_FUNC_MASK ) {
                    case 0:
                    {
                        if ( (0 == fd) && (MIPS_SPEC_COP_MASK_MTC == fr) ) {
                            bundle->addInstruction(
                                new VanadisGPR2FPInstruction<int32_t, int32_t>(ins_addr, hw_thr, options, fpflags, fs, rt));
                            insertDecodeFault = false;
                        }
                        else if ( (0 == fd) && (MIPS_SPEC_COP_MASK_MFC == fr) ) {
                            bundle->addInstruction(
                                new VanadisFP2GPRInstruction<int32_t, int32_t>(ins_addr, hw_thr, options, fpflags, rt, fs));
                            insertDecodeFault = false;
                        }
                        else if ( (0 == fd) && (MIPS_SPEC_COP_MASK_CF == fr) ) {
                            uint16_t fp_ctrl_reg = 0;
                            bool     fp_matched  = false;

                            switch ( rd ) {
                            case 0:
                                fp_ctrl_reg = MIPS_FP_VER_REG;
                                fp_matched  = true;
                                break;
                            case 31:
                                fp_ctrl_reg = MIPS_FP_STATUS_REG;
                                fp_matched  = true;
                                break;
                            default:
                                break;
                            }

                            if ( fp_matched ) {
                                bundle->addInstruction(new VanadisFP2GPRInstruction<int32_t, int32_t>(
                                    ins_addr, hw_thr, options, fpflags, rt, fp_ctrl_reg));
                                insertDecodeFault = false;
                            }
                        }
                        else if ( (0 == fd) && (MIPS_SPEC_COP_MASK_CT == fr) ) {
                            uint16_t fp_ctrl_reg = 0;
                            bool     fp_matched  = false;

                            switch ( rd ) {
                            case 0:
                                fp_ctrl_reg = MIPS_FP_VER_REG;
                                fp_matched  = true;
                                break;
                            case 31:
                                fp_ctrl_reg = MIPS_FP_STATUS_REG;
                                fp_matched  = true;
                                break;
                            default:
                                break;
                            }

                            if ( fp_matched ) {
                                bundle->addInstruction(new VanadisGPR2FPInstruction<int32_t, int32_t>(
                                    ins_addr, hw_thr, options, fpflags, fp_ctrl_reg, rt));
                                insertDecodeFault = false;
                            }
                        }
                        else {
                            switch ( fr ) {
                            case 16:
                                bundle->addInstruction(
                                    new VanadisFPAddInstruction<float>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                                insertDecodeFault = false;
                                break;
                            case 17:
                                bundle->addInstruction(
                                    new VanadisFPAddInstruction<double>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                                insertDecodeFault = false;
                                break;
                            case 20:
                                bundle->addInstruction(
                                    new VanadisFPAddInstruction<int32_t>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                                insertDecodeFault = false;
                                break;
                            case 21:
                                bundle->addInstruction(
                                    new VanadisFPAddInstruction<int64_t>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                                insertDecodeFault = false;
                                break;
                            default:
                                break;
                            }
                        }
                    } break;

                    case MIPS_SPEC_COP_MASK_MOV:
                    {
                        // Decide operand format
                        switch ( fr ) {
                        case 16:
                        {
                            bundle->addInstruction(
                                new VanadisFP2FPInstruction<int32_t>(
                                    ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                        } break;
                        case 17:
                        {
                            bundle->addInstruction(
                                new VanadisFP2FPInstruction<int64_t>(
                                    ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                        } break;
                        }

                        MIPS_INC_DECODE_STAT(stat_decode_cop1_mov);
                    } break;

                    case MIPS_SPEC_COP_MASK_MUL:
                    {
                        switch ( fr ) {
                        case 16:
                            bundle->addInstruction(
                                new VanadisFPMultiplyInstruction<float>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        case 17:
                            bundle->addInstruction(
                                new VanadisFPMultiplyInstruction<double>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        case 20:
                            bundle->addInstruction(
                                new VanadisFPMultiplyInstruction<int32_t>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        case 21:
                            bundle->addInstruction(
                                new VanadisFPMultiplyInstruction<int64_t>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        default:
                            break;
                        }

                        MIPS_INC_DECODE_STAT(stat_decode_cop1_mul);
                    } break;

                    case MIPS_SPEC_COP_MASK_DIV:
                    {
                        switch ( fr ) {
                        case 16:
                            bundle->addInstruction(
                                new VanadisFPDivideInstruction<float>(
                                    ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        case 17:
                            bundle->addInstruction(
                                new VanadisFPDivideInstruction<double>(
                                    ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        case 20:
                            bundle->addInstruction(
                                new VanadisFPDivideInstruction<int32_t>(
                                    ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        case 21:
                            bundle->addInstruction(
                                new VanadisFPDivideInstruction<int64_t>(
                                    ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;

                            break;
                        default:
                            break;
                        }

                        MIPS_INC_DECODE_STAT(stat_decode_cop1_div);
                    } break;

                    case MIPS_SPEC_COP_MASK_SUB:
                    {
                        switch ( fr ) {
                        case 16:
                            bundle->addInstruction(
                                new VanadisFPSubInstruction<float>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        case 17:
                            bundle->addInstruction(
                                new VanadisFPSubInstruction<double>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        case 20:
                            bundle->addInstruction(
                                new VanadisFPSubInstruction<int32_t>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        case 21:
                            bundle->addInstruction(
                                new VanadisFPSubInstruction<int64_t>(ins_addr, hw_thr, options, fpflags, fd, fs, ft));
                            insertDecodeFault = false;
                            break;
                        default:
                            break;
                        }

                        MIPS_INC_DECODE_STAT(stat_decode_cop1_sub);
                    } break;

                    case MIPS_SPEC_COP_MASK_CVTS:
                    {
                        switch ( fr ) {
                        case 16:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<float, float>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        case 17:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<double, float>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        case 20:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<int32_t, float>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        case 21:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<int64_t, float>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        default:
                            break;
                        }

                        MIPS_INC_DECODE_STAT(stat_decode_cop1_cvts);
                    } break;

                    case MIPS_SPEC_COP_MASK_CVTD:
                    {
                        switch ( fr ) {
                        case 16:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<float, double>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        case 17:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<double, double>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        case 20:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<int32_t, double>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        case 21:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<int64_t, double>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        default:
                            break;
                        }

                        MIPS_INC_DECODE_STAT(stat_decode_cop1_cvtd);
                    }

                    break;

                    case MIPS_SPEC_COP_MASK_CVTW:
                    {
                        switch ( fr ) {
                        case 16:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<float, int32_t>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        case 17:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<double, int32_t>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        case 20:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<int32_t, int32_t>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        case 21:
                            bundle->addInstruction(
                                new VanadisFPConvertInstruction<int64_t, int32_t>(ins_addr, hw_thr, options, fpflags, fd, fs));
                            insertDecodeFault = false;
                            break;
                        default:
                            break;
                        }

                        MIPS_INC_DECODE_STAT(stat_decode_cop1_cvtw);
                    }

                    break;

                    case MIPS_SPEC_COP_MASK_CMP_LT:
                    case MIPS_SPEC_COP_MASK_CMP_LTE:
                    case MIPS_SPEC_COP_MASK_CMP_EQ:
                    {
                        // if neither are true, then we have a good decode, otherwise a
                        // problem. register 31 is where condition codes and rounding modes
                        // are kept

                        switch ( fr ) {
                        case 16:
                        {
                            switch ( next_ins & 0xF ) {
                            case 0x2:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_eq);

                                bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_EQ, float>(
                                    ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            case 0xC:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_lt);

                                bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LT, float>(
                                    ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            case 0xE:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_lte);

                                bundle->addInstruction(
                                    new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LTE, float>(
                                        ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            default:
                                break;
                            }
                        } break;
                        case 17:
                        {
                            switch ( next_ins & 0xF ) {
                            case 0x2:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_eq);

                                bundle->addInstruction(
                                    new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_EQ, double>(
                                        ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            case 0xC:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_lt);

                                bundle->addInstruction(
                                    new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LT, double>(
                                        ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            case 0xE:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_lte);

                                bundle->addInstruction(
                                    new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LTE, double>(
                                        ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            default:
                                break;
                            }
                        } break;
                        case 20:
                        {
                            switch ( next_ins & 0xF ) {
                            case 0x2:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_eq);

                                bundle->addInstruction(
                                    new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_EQ, int32_t>(
                                        ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            case 0xC:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_lt);

                                bundle->addInstruction(
                                    new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LT, int32_t>(
                                        ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            case 0xE:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_lte);

                                bundle->addInstruction(
                                    new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LTE, int32_t>(
                                        ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            default:
                                break;
                            }
                        } break;
                        case 21:
                        {
                            switch ( next_ins & 0xF ) {
                            case 0x2:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_eq);

                                bundle->addInstruction(
                                    new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_EQ, int64_t>(
                                        ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            case 0xC:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_lt);

                                bundle->addInstruction(
                                    new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_EQ, int64_t>(
                                        ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            case 0xE:
                                MIPS_INC_DECODE_STAT(stat_decode_cop1_lte);

                                bundle->addInstruction(
                                    new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LTE, int64_t>(
                                        ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, fs, ft));
                                insertDecodeFault = false;

                                break;
                            default:
                                break;
                            }
                        } break;
                        default:
                            break;
                        }
                    } break;
                    }
                }
            } break;

            case MIPS_SPEC_OP_SPECIAL3:
            {
                //				output->verbose(CALL_INFO, 16, 0, "[decoder,
                // partial: special3], further decode required...\n");

                switch ( next_ins & 0x3F ) {
                case MIPS_SPEC_OP_MASK_RDHWR:
                {
                    const uint16_t target_reg = rt;
                    const uint16_t req_type   = rd;

                    //						output->verbose(CALL_INFO, 16, 0,
                    //"[decode/RDHWR] target: %" PRIu16 " type: %" PRIu16 "\n",
                    //							target_reg,
                    // req_type);

                    switch ( rd ) {
                    case 29:
                        bundle->addInstruction(
                            new VanadisSetRegisterInstruction<int32_t>(
                                ins_addr, hw_thr, options, target_reg, static_cast<int32_t>(getThreadLocalStoragePointer())));
                        insertDecodeFault = false;
                        break;
                    }
                } break;
                }
            } break;

            default:
                break;
            }
        }

        if ( insertDecodeFault ) {
            bundle->addInstruction(new VanadisInstructionDecodeFault(ins_addr, getHardwareThread(), options));
            stat_decode_fault->addData(1);
        }
        else {
            stat_uop_generated->addData(bundle->getInstructionCount());
        }

        for ( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
            output->verbose(
                CALL_INFO, 16, 0, "-> [%3" PRIu32 "]: %s\n", i, bundle->getInstructionByIndex(i)->getInstCode());
        }

        // Mark the end of a micro-op group so we can count real instructions and
        // not just micro-ops
        if ( bundle->getInstructionCount() > 0 ) {
            bundle->getInstructionByIndex(bundle->getInstructionCount() - 1)->markEndOfMicroOpGroup();
        }
    }

    const VanadisDecoderOptions* options;

    bool haltOnDecodeZero;

    uint16_t icache_max_bytes_per_cycle;
    uint16_t max_decodes_per_cycle;
    uint16_t decode_buffer_max_entries;

    Statistic<uint64_t>* stat_decode_add;
    Statistic<uint64_t>* stat_decode_addu;
    Statistic<uint64_t>* stat_decode_and;
    Statistic<uint64_t>* stat_decode_dadd;
    Statistic<uint64_t>* stat_decode_daddu;
    Statistic<uint64_t>* stat_decode_ddiv;
    Statistic<uint64_t>* stat_decode_div;
    Statistic<uint64_t>* stat_decode_divu;
    Statistic<uint64_t>* stat_decode_dmult;
    Statistic<uint64_t>* stat_decode_dmultu;
    Statistic<uint64_t>* stat_decode_dsllv;
    Statistic<uint64_t>* stat_decode_dsrav;
    Statistic<uint64_t>* stat_decode_dsrlv;
    Statistic<uint64_t>* stat_decode_dsub;
    Statistic<uint64_t>* stat_decode_dsubu;
    Statistic<uint64_t>* stat_decode_jr;
    Statistic<uint64_t>* stat_decode_jalr;
    Statistic<uint64_t>* stat_decode_mfhi;
    Statistic<uint64_t>* stat_decode_mflo;
    Statistic<uint64_t>* stat_decode_mult;
    Statistic<uint64_t>* stat_decode_multu;
    Statistic<uint64_t>* stat_decode_nor;
    Statistic<uint64_t>* stat_decode_or;
    Statistic<uint64_t>* stat_decode_sllv;
    Statistic<uint64_t>* stat_decode_slt;
    Statistic<uint64_t>* stat_decode_sltu;
    Statistic<uint64_t>* stat_decode_srav;
    Statistic<uint64_t>* stat_decode_srlv;
    Statistic<uint64_t>* stat_decode_sub;
    Statistic<uint64_t>* stat_decode_subu;
    Statistic<uint64_t>* stat_decode_syscall;
    Statistic<uint64_t>* stat_decode_sync;
    Statistic<uint64_t>* stat_decode_xor;
    Statistic<uint64_t>* stat_decode_sll;
    Statistic<uint64_t>* stat_decode_srl;
    Statistic<uint64_t>* stat_decode_sra;
    Statistic<uint64_t>* stat_decode_bltz;
    Statistic<uint64_t>* stat_decode_bgezal;
    Statistic<uint64_t>* stat_decode_bgez;
    Statistic<uint64_t>* stat_decode_lui;
    Statistic<uint64_t>* stat_decode_lb;
    Statistic<uint64_t>* stat_decode_lbu;
    Statistic<uint64_t>* stat_decode_lhu;
    Statistic<uint64_t>* stat_decode_lw;
    Statistic<uint64_t>* stat_decode_lfp32;
    Statistic<uint64_t>* stat_decode_ll;
    Statistic<uint64_t>* stat_decode_lwl;
    Statistic<uint64_t>* stat_decode_lwr;
    Statistic<uint64_t>* stat_decode_sb;
    Statistic<uint64_t>* stat_decode_sc;
    Statistic<uint64_t>* stat_decode_sw;
    Statistic<uint64_t>* stat_decode_sh;
    Statistic<uint64_t>* stat_decode_sfp32;
    Statistic<uint64_t>* stat_decode_swr;
    Statistic<uint64_t>* stat_decode_swl;
    Statistic<uint64_t>* stat_decode_addiu;
    Statistic<uint64_t>* stat_decode_beq;
    Statistic<uint64_t>* stat_decode_bgtz;
    Statistic<uint64_t>* stat_decode_blez;
    Statistic<uint64_t>* stat_decode_bne;
    Statistic<uint64_t>* stat_decode_slti;
    Statistic<uint64_t>* stat_decode_sltiu;
    Statistic<uint64_t>* stat_decode_andi;
    Statistic<uint64_t>* stat_decode_ori;
    Statistic<uint64_t>* stat_decode_j;
    Statistic<uint64_t>* stat_decode_jal;
    Statistic<uint64_t>* stat_decode_xori;
    Statistic<uint64_t>* stat_decode_rdhwr;
    Statistic<uint64_t>* stat_decode_cop1_mtc;
    Statistic<uint64_t>* stat_decode_cop1_mfc;
    Statistic<uint64_t>* stat_decode_cop1_cf;
    Statistic<uint64_t>* stat_decode_cop1_ct;
    Statistic<uint64_t>* stat_decode_cop1_mov;
    Statistic<uint64_t>* stat_decode_cop1_mul;
    Statistic<uint64_t>* stat_decode_cop1_div;
    Statistic<uint64_t>* stat_decode_cop1_sub;
    Statistic<uint64_t>* stat_decode_cop1_cvts;
    Statistic<uint64_t>* stat_decode_cop1_cvtd;
    Statistic<uint64_t>* stat_decode_cop1_cvtw;
    Statistic<uint64_t>* stat_decode_cop1_lt;
    Statistic<uint64_t>* stat_decode_cop1_lte;
    Statistic<uint64_t>* stat_decode_cop1_eq;
};

} // namespace Vanadis
} // namespace SST

#endif
