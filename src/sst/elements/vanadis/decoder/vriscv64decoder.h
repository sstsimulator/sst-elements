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

#ifndef _H_VANADIS_RISCV64_DECODER
#define _H_VANADIS_RISCV64_DECODER

#include "decoder/vdecoder.h"
#include "inst/vinstall.h"
#include "os/vriscvcpuos.h"

#include <cstdint>
#include <cstring>

#define VANADIS_RISCV_OPCODE_MASK 0x7F
#define VANADIS_RISCV_RD_MASK     0xF80
#define VANADIS_RISCV_RS1_MASK    0xF8000
#define VANADIS_RISCV_RS2_MASK    0x1F00000
#define VANADIS_RISCV_FUNC3_MASK  0x7000
#define VANADIS_RISCV_FUNC7_MASK  0xFE000000
#define VANADIS_RISCV_IMM12_MASK  0xFFF00000
#define VANADIS_RISCV_IMM7_MASK   0xFE000000
#define VANADIS_RISCV_IMM5_MASK   0xF80
#define VANADIS_RISCV_IMM20_MASK  0xFFFFF000

#define VANADIS_RISCV_SIGN12_MASK       0x800
#define VANADIS_RISCV_SIGN12_UPPER_1_32 0xFFFFF000
#define VANADIS_RISCV_SIGN12_UPPER_1_64 0xFFFFFFFFFFFFF000LL

namespace SST {
namespace Vanadis {

class VanadisRISCV64Decoder : public VanadisDecoder
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
      VanadisRISCV64Decoder, "vanadis", "VanadisRISCV64Decoder",
      SST_ELI_ELEMENT_VERSION(1, 0, 0),
      "Implements a RISCV64-compatible decoder for Vanadis CPU processing.",
      SST::Vanadis::VanadisDecoder)
    SST_ELI_DOCUMENT_PARAMS(
      {"decode_max_ins_per_cycle", "Maximum number of instructions that can be "
                                   "decoded and issued per cycle"},
      {"uop_cache_entries", "Number of micro-op cache entries, this "
                            "corresponds to ISA-level instruction counts."},
      {"predecode_cache_entries",
       "Number of cache lines that a cached prior to decoding (these support "
       "loading from cache prior to decode)"},
      {"halt_on_decode_fault",
		"Fatal error if a decode fault occurs, used for debugging and not recommmended default is 0 (false)", "0"})

    VanadisRISCV64Decoder(ComponentId_t id, Params& params) : VanadisDecoder(id, params)
    {
        // we need TWO additional registers for AMO microcode operations, RISC-V has 32 + 2 int for our micro-code.
        options = new VanadisDecoderOptions(static_cast<uint16_t>(0), 34, 32, 2, VANADIS_REGISTER_MODE_FP64);
        max_decodes_per_cycle = params.find<uint16_t>("decode_max_ins_per_cycle", 2);

        // See if we get an entry point the sub-component says we have to use
        // if not, we will fall back to ELF reading at the core level to work this
        // out
        setInstructionPointer(params.find<uint64_t>("entry_point", 0));

        fatal_decode_fault = params.find<bool>("halt_on_decode_fault", false);
    }

    ~VanadisRISCV64Decoder() {}

    const char*                  getISAName() const override { return "RISCV64"; }
    uint16_t                     countISAIntReg() const override { return options->countISAIntRegisters(); }
    uint16_t                     countISAFPReg() const override { return options->countISAFPRegisters(); }
    const VanadisDecoderOptions* getDecoderOptions() const override { return options; }
    VanadisFPRegisterMode        getFPRegisterMode() const override { return VANADIS_REGISTER_MODE_FP64; }

    void setStackPointer( SST::Output* output, VanadisISATable* isa_tbl,
	VanadisRegisterFile* regFile, const uint64_t start_stack_address ) override {

        output->verbose(
            CALL_INFO, 16, 0, "-> Setting SP to (64B-aligned):          %" PRIu64 " / 0x%0llx\n", start_stack_address,
            start_stack_address);

        // Per RISCV Assembly Programemr's handbook, register x2 is for stack
        // pointer
        const int16_t sp_phys_reg = isa_tbl->getIntPhysReg(2);

        output->verbose(CALL_INFO, 16, 0, "-> Stack Pointer (r29) maps to phys-reg: %" PRIu16 "\n", sp_phys_reg);

        // Setup the initial stack pointer
        regFile->setIntReg<uint64_t>(sp_phys_reg, start_stack_address);
    }

    void tick(SST::Output* output, uint64_t cycle) override
    {
        output->verbose(CALL_INFO, 16, 0, "-> Decode step for thr: %" PRIu32 "\n", hw_thr);
        output->verbose(CALL_INFO, 16, 0, "---> Max decodes per cycle: %" PRIu16 "\n", max_decodes_per_cycle);

        for ( uint16_t i = 0; i < max_decodes_per_cycle; ++i ) {
            if ( !thread_rob->full() ) {
                if ( ins_loader->hasBundleAt(ip) ) {
                    // We have the instruction in our micro-op cache
                    output->verbose(
                        CALL_INFO, 16, 0, "---> Found uop bundle for ip=0x%llx, loading from cache...\n", ip);
                    stat_uop_hit->addData(1);

                    VanadisInstructionBundle* bundle = ins_loader->getBundleAt(ip);
                    output->verbose(
                        CALL_INFO, 16, 0, "----> Bundle contains %" PRIu32 " entries.\n",
                        bundle->getInstructionCount());

                    // Do we have enough space in the ROB to push the micro-op bundle into
                    // the queue?
                    if ( bundle->getInstructionCount() < (thread_rob->capacity() - thread_rob->size()) ) {
                        bool bundle_has_branch = false;

                        for ( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
                            VanadisInstruction* next_ins = bundle->getInstructionByIndex(i);

                            if ( next_ins->getInstFuncType() == INST_BRANCH ) {
                                VanadisSpeculatedInstruction* next_spec_ins =
                                    dynamic_cast<VanadisSpeculatedInstruction*>(next_ins);

                                if ( branch_predictor->contains(ip) ) {
                                    // We have an address predicton from the branching unit
                                    const uint64_t predicted_address = branch_predictor->predictAddress(ip);
                                    next_spec_ins->setSpeculatedAddress(predicted_address);

                                    output->verbose(
                                        CALL_INFO, 16, 0,
                                        "----> contains a branch: 0x%llx / predicted "
                                        "(found in predictor): 0x%llx\n",
                                        ip, predicted_address);

                                    ip                = predicted_address;
                                    bundle_has_branch = true;
                                }
                                else {
                                    // We don't have an address prediction
                                    // so just speculate that we are going to drop through to the
                                    // next instruction as we aren't sure where this will go yet

                                    output->verbose(
                                        CALL_INFO, 16, 0,
                                        "----> contains a branch: 0x%llx / predicted "
                                        "(not-found in predictor): 0x%llx, pc-increment: %" PRIu64 "\n",
                                        ip, ip + 4, bundle->pcIncrement());

                                    ip += bundle->pcIncrement();
                                    next_spec_ins->setSpeculatedAddress(ip);
                                    bundle_has_branch = true;
                                }
                            }

                            thread_rob->push(next_ins->clone());
                        }

                        // Move to the next address, if we had a branch we should have
                        // already found a predicted target addeess to decode
                        output->verbose(
                            CALL_INFO, 16, 0, "----> branch? %s, ip=0x%llx + inc=%" PRIu64 " = new-ip=0x%llx\n",
                            bundle_has_branch ? "yes" : "no", ip, bundle_has_branch ? 0 : bundle->pcIncrement(),
                            bundle_has_branch ? ip : ip + bundle->pcIncrement());
                        ip = bundle_has_branch ? ip : ip + bundle->pcIncrement();
                    }
                    else {
                        output->verbose(
                            CALL_INFO, 16, 0, "----> Not enough space in the ROB, will stall this cycle.\n");
                        stat_uop_delayed_rob_full->addData(1);
                    }
                }
                else if ( ins_loader->hasPredecodeAt(ip, 4) ) {
                    // We have a loaded instruction cache line but have not decoded it yet
                    output->verbose(
                        CALL_INFO, 16, 0,
                        "---> uop not found, but is located in the predecode "
                        "i0-icache (ip=0x%llx)\n",
                        ip);
                    VanadisInstructionBundle* decoded_bundle = new VanadisInstructionBundle(ip);
                    stat_predecode_hit->addData(1);

                    uint32_t temp_ins = 0;

                    const bool predecode_bytes =
                        ins_loader->getPredecodeBytes(output, ip, (uint8_t*)&temp_ins, sizeof(temp_ins));

                    if ( predecode_bytes ) {
                        output->verbose(CALL_INFO, 16, 0, "---> performing a decode for ip=0x%llx\n", ip);
                        decode(output, ip, temp_ins, decoded_bundle);

                        output->verbose(
                            CALL_INFO, 16, 0, "---> bundle generates %" PRIu32 " micro-ops\n",
                            (uint32_t)decoded_bundle->getInstructionCount());

                        ins_loader->cacheDecodedBundle(decoded_bundle);

                        if ( 0 == decoded_bundle->getInstructionCount() ) {
                            output->fatal(CALL_INFO, -1, "Error - bundle at: 0x%llx generates no micro-ops.\n", ip);
                        }

                        // Exit this cycle because results saved to cache are available next
                        // cycle
                        break;
                    }
                    else {
                        output->fatal(
                            CALL_INFO, -1,
                            "Error - predecoded bytes for 0x%llu found, but "
                            "retrieval of bytes failed.\n",
                            ip);
                    }
                }
                else {
                    // Not in micro or predecode cache, so we have to regenrata a request
                    // and stop further processing
                    output->verbose(
                        CALL_INFO, 16, 0,
                        "---> microop bundle and pre-decoded bytes are not found for "
                        "0x%llx, requested read for cache line (line=%" PRIu64 ")\n",
                        ip, ins_loader->getCacheLineWidth());
                    ins_loader->requestLoadAt(output, ip, 4);
                    stat_ins_bytes_loaded->addData(4);
                    stat_predecode_miss->addData(1);
                    break;
                }
            }
            else {
                output->verbose(
                    CALL_INFO, 16, 0,
                    "---> Decode pending queue (ROB) is full, no more "
                    "decoded permitted this cycle.\n");
            }
        }

        output->verbose(CALL_INFO, 16, 0, "---> cycle is completed, ip=0x%llx\n", ip);
    }

protected:
    const VanadisDecoderOptions* options;
    bool                         fatal_decode_fault;
    uint16_t                     icache_max_bytes_per_cycle;
    uint16_t                     max_decodes_per_cycle;
    uint16_t                     decode_buffer_max_entries;

    void decode(SST::Output* output, const uint64_t ins_address, const uint32_t ins, VanadisInstructionBundle* bundle)
    {
        output->verbose(CALL_INFO, 16, 0, "[decode] -> addr: 0x%llx / ins: 0x%08x\n", ins_address, ins);
        output->verbose(CALL_INFO, 16, 0, "[decode] -> ins-bytes: 0x%08x\n", ins);

        // We are supposed to have 16b packets for RISCV instructions, if we don't then mark fault
        if ( (ins_address & 0x1) != 0 ) {
            bundle->addInstruction(new VanadisInstructionDecodeAlignmentFault(ins_address, hw_thr, options));
            return;
        }

        uint32_t op_code = extract_opcode(ins);

        uint16_t rd  = 0;
        uint16_t rs1 = 0;
        uint16_t rs2 = 0;
        uint16_t rs3 = 0;

        uint32_t func_code  = 0;
        uint32_t func_code3 = 0;
        uint32_t func_code7 = 0;
        uint64_t uimm64     = 0;
        int64_t  simm64     = 0;

        bool decode_fault = true;


        // if the last two bits that are set are 11, then we are performing at least 32bit instruction formats,
        // otherwise we are performing decodes on the C-extension (16b) formats
        if ( (ins & 0x3) == 0x3 ) {
            output->verbose(
                CALL_INFO, 16, 0, "[decode] -> 32bit format / ins-op-code-family: %" PRIu32 " / 0x%x\n", op_code,
                op_code);

            switch ( op_code ) {
            case 0x3:
            {
                // Load data
                processI<int64_t>(ins, op_code, rd, rs1, func_code, simm64);

                switch ( func_code ) {
                case 0:
                {
                    // LB
                    output->verbose(
                        CALL_INFO, 16, 0, "----> LB %" PRIu16 " <- %" PRIu16 " %" PRId64 "\n", rd, rs1, simm64);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 1, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 1:
                {
                    // LH
                    output->verbose(
                        CALL_INFO, 16, 0, "----> LH %" PRIu16 " <- %" PRIu16 " %" PRId64 "\n", rd, rs1, simm64);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 2, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 2:
                {
                    // LW
                    output->verbose(
                        CALL_INFO, 16, 0, "----> LW %" PRIu16 " <- %" PRIu16 " %" PRId64 "\n", rd, rs1, simm64);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 4, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 3:
                {
                    // LD
                    output->verbose(
                        CALL_INFO, 16, 0, "----> LD %" PRIu16 " <- %" PRIu16 " %" PRId64 "\n", rd, rs1, simm64);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 8, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 4:
                {
                    // LBU
                    output->verbose(
                        CALL_INFO, 16, 0, "----> LBU %" PRIu16 " <- %" PRIu16 " %" PRId64 "\n", rd, rs1, simm64);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 1, false, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 5:
                {
                    // LHU
                    output->verbose(
                        CALL_INFO, 16, 0, "----> LHU %" PRIu16 " <- %" PRIu16 " %" PRId64 "\n", rd, rs1, simm64);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 2, false, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 6:
                {
                    // LWU
                    output->verbose(
                        CALL_INFO, 16, 0, "----> LWU %" PRIu16 " <- %" PRIu16 " %" PRId64 "\n", rd, rs1, simm64);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 4, false, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                }
            } break;
            case 0x7:
            {
                processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);

                // Floating point load
                switch ( func_code3 ) {
                case 0x2:
                {
                    // FLW
                    output->verbose(
                        CALL_INFO, 16, 0, "----> FLW %" PRIu16 " <- memory[ %" PRIu16 " + %" PRId64 " ]\n", rd, rs1,
                        simm64);
                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 4, true, MEM_TRANSACTION_NONE,
                        LOAD_FP_REGISTER));
                    decode_fault = false;
                } break;
                case 0x3:
                {
                    // FLD
                    output->verbose(
                        CALL_INFO, 16, 0, "----> FLD %" PRIu16 " <- memory[ %" PRIu16 " + %" PRId64 " ]\n", rd, rs1,
                        simm64);
                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 8, true, MEM_TRANSACTION_NONE,
                        LOAD_FP_REGISTER));
                    decode_fault = false;
                } break;
                }
            } break;
            case 0x23:
            {
                // Store data
                processS<int64_t>(ins, op_code, rs1, rs2, func_code3, simm64);

                if ( func_code < 4 ) {
                    // shift to get the power of 2 number of bytes to store
                    const uint32_t store_bytes = 1 << func_code3;

                    output->verbose(
                        CALL_INFO, 16, 0,
                        "----> STORE width: %" PRIu32 " bytes == (1 << %" PRIu32 ") %" PRIu16 " -> memory[ %" PRIu16
                        " + %" PRId64 " / (0x%llx)]\n",
                        store_bytes, func_code3, rs2, rs1, simm64, simm64);

                    bundle->addInstruction(new VanadisStoreInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rs2, store_bytes, MEM_TRANSACTION_NONE,
                        STORE_INT_REGISTER));
                    decode_fault = false;
                }
            } break;
            case 0x13:
            {
                // Immediate arithmetic
                func_code = extract_func3(ins);

                output->verbose(CALL_INFO, 16, 0, "----> immediate-arith func: %" PRIu32 "\n", func_code);

                switch ( func_code ) {
                case 0:
                {
                    // ADDI
                    processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);

                    output->verbose(
                        CALL_INFO, 16, 0, "------> ADDI %" PRIu16 " <- %" PRIu16 " + %" PRId64 "\n", rd, rs1, simm64);

                    bundle->addInstruction(new VanadisAddImmInstruction<int64_t>(
                        ins_address, hw_thr, options, rd, rs1, simm64));
                    decode_fault = false;
                } break;
                case 1:
                {
                    // SLLI
                    rd  = extract_rd(ins);
                    rs1 = extract_rs1(ins);

                    uint32_t func_code6 = (ins & 0xFC000000);
                    uint32_t shift_by   = (ins & 0x3F00000) >> 20;

                    output->verbose(
                        CALL_INFO, 16, 0, "------> func_code6 = %" PRIu32 " / shift = %" PRIu32 " (0x%" PRIx32 ")\n",
                        func_code6, shift_by, shift_by);

                    switch ( func_code6 ) {
                    case 0x0:
                    {
                        bundle->addInstruction(
                            new VanadisShiftLeftLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, shift_by));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 2:
                {
                    // SLTI
                    processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);
                    bundle->addInstruction(new VanadisSetRegCompareImmInstruction<
                                           REG_COMPARE_LT, VanadisRegisterFormat::VANADIS_FORMAT_INT64, true>(
                        ins_address, hw_thr, options, rd, rs1, simm64));
                    decode_fault = false;
                } break;
                case 3:
                {
                    // SLTIU
                    processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);
                    bundle->addInstruction(new VanadisSetRegCompareImmInstruction<
                                           REG_COMPARE_LT, VanadisRegisterFormat::VANADIS_FORMAT_INT64, false>(
                        ins_address, hw_thr, options, rd, rs1, simm64));
                    decode_fault = false;
                } break;
                case 4:
                {
                    // XORI
                    processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);
                    bundle->addInstruction(new VanadisXorImmInstruction(ins_address, hw_thr, options, rd, rs1, simm64));
                    decode_fault = false;
                } break;
                case 5:
                {
                    // CHECK SRLI / SRAI
                    rd  = extract_rd(ins);
                    rs1 = extract_rs1(ins);

                    uint32_t func_code6 = (ins & 0xFC000000);
                    uint32_t shift_by   = (ins & 0x3F00000) >> 20;

                    output->verbose(
                        CALL_INFO, 16, 0, "------> func_code6 = %" PRIu32 " / shift = %" PRIu32 " (0x%" PRIx32 ")\n",
                        func_code6, shift_by, shift_by);

                    switch ( func_code6 ) {
                    case 0x0:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "--------> SRLI %" PRIu16 " <- %" PRIu16 " >> %" PRIu32 "\n", rd, rs1,
                            shift_by);
                        bundle->addInstruction(
                            new VanadisShiftRightLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, shift_by));
                        decode_fault = false;
                    } break;
                    case 0x40000000:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "--------> SRAI %" PRIu16 " <- %" PRIu16 " >> %" PRIu32 "\n", rd, rs1,
                            shift_by);
                        bundle->addInstruction(
                            new VanadisShiftRightArithmeticImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, shift_by));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 6:
                {
                    // ORI
                    processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);
                    bundle->addInstruction(new VanadisOrImmInstruction(ins_address, hw_thr, options, rd, rs1, simm64));
                    decode_fault = false;
                } break;
                case 7:
                {
                    // ANDI
                    processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);
                    bundle->addInstruction(new VanadisAndImmInstruction(ins_address, hw_thr, options, rd, rs1, simm64));
                    decode_fault = false;
                } break;
                };
            } break;
            case 0x33:
            {
                // integer arithmetic/logical
                processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                output->verbose(
                    CALL_INFO, 16, 0, "-----> decode R-type, func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n",
                    func_code3, func_code7);

                switch ( func_code3 ) {
                case 0:
                {
                    switch ( func_code7 ) {
                    case 0:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> ADD %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n", rd, rs1, rs2);
                        // ADD
                        bundle->addInstruction(
                            new VanadisAddInstruction<int64_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        // MUL
                        // TODO - check register ordering
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> MUL %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n", rd, rs1, rs2);

                        bundle->addInstruction(
                            new VanadisMultiplyInstruction<int64_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x20:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> SUB %" PRIu16 " <- %" PRIu16 " - %" PRIu16 "\n", rd, rs1, rs2);
                        // SUB
                        bundle->addInstruction(new VanadisSubInstruction<int64_t>(
                            ins_address, hw_thr, options, rd, rs1, rs2, false));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 1:
                {
                    switch ( func_code7 ) {
                    case 0:
                    {
                        // SLL
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> SLL %" PRIu16 " <- %" PRIu16 " << %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(
                            new VanadisShiftLeftLogicalInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        // MULH
                        // NOT SURE WHAT THIS IS?
                    } break;
                    };
                } break;
                case 2:
                {
                    switch ( func_code7 ) {
                    case 0:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> SLT %" PRIu16 " <-  %" PRIu16 " < %" PRIu16 "\n", rd, rs1,
                            rs2);
                        // SLT
                        bundle->addInstruction(
                            new VanadisSetRegCompareInstruction<
                                VanadisRegisterCompareType::REG_COMPARE_LT, VanadisRegisterFormat::VANADIS_FORMAT_INT64,
                                true>(ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        // MULHSU
                        // NOT SURE WHAT THIS IS?
                    } break;
                    };
                } break;
                case 3:
                {
                    switch ( func_code7 ) {
                    case 0x0:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> SLTU %" PRIu16 " <-  %" PRIu16 " < %" PRIu16 "\n", rd, rs1,
                            rs2);
                        // SLTU
                        bundle->addInstruction(
                            new VanadisSetRegCompareInstruction<
                                VanadisRegisterCompareType::REG_COMPARE_LT, VanadisRegisterFormat::VANADIS_FORMAT_INT64,
                                false>(ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x1:
                    {
                        // MULHU mul und place in upper XLEN bits, unsigned 
                        // need to check the register order
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> MULHU %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n", rd, rs1, rs2);

                        bundle->addInstruction(
                            new VanadisMultiplyHighInstruction<uint64_t,uint64_t>( ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 4:
                {
                    switch ( func_code7 ) {
                    case 0x0:
                    {
                        // XOR
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> XOR %" PRIu16 " <-  %" PRIu16 " ^ %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(new VanadisXorInstruction(ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        // DIV
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> DIV %" PRIu16 " <-  %" PRIu16 " / %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(
                            new VanadisDivideInstruction<int64_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 5:
                {
                    switch ( func_code7 ) {
                    case 0:
                    {
                        // SRL
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> SRL %" PRIu16 " <-  %" PRIu16 " >> %" PRIu16 "\n", rd, rs1,
                            rs2);
                        bundle->addInstruction(
                            new VanadisShiftRightLogicalInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        // DIVU
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> DIVU %" PRIu16 " <-  %" PRIu16 " / %" PRIu16 "\n", rd, rs1,
                            rs2);
                        bundle->addInstruction(
                            new VanadisDivideInstruction<uint64_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 32:
                    {
                        // SRA
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> SRA %" PRIu16 " <- %" PRIu16 " >> %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(
                            new VanadisShiftRightArithmeticInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 6:
                {
                    switch ( func_code7 ) {
                    case 0x0:
                    {
                        // OR
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> OR %" PRIu16 " <- %" PRIu16 " | %" PRIu16 "\n", rd, rs1, rs2);

                        bundle->addInstruction(new VanadisOrInstruction(ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x1:
                    {
                        // REM
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> REM %" PRIu16 " <- %" PRIu16 " %% %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(
                            new VanadisModuloInstruction<int64_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    }
                } break;
                case 7:
                {
                    switch ( func_code7 ) {
                    case 0x0:
                    {
                        // AND
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> AND %" PRIu16 " <- %" PRIu16 " & %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(new VanadisAndInstruction(ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x1:
                    {
                        // REMU
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> REMU %" PRIu16 " <- %" PRIu16 " %% %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(
                            new VanadisModuloInstruction<uint64_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    }
                } break;
                };
            } break;
            case 0x37:
            {
                // LUI
					 int32_t uimm32 = 0;
                processU<int32_t>(ins, op_code, rd, uimm32);;

					 output->verbose(CALL_INFO, 16, 0, "----> LUI %" PRIu16 " <- 0x%lx\n", rd, uimm32);

                bundle->addInstruction(new VanadisSetRegisterInstruction<int32_t>(
                    ins_address, hw_thr, options, rd, uimm32));
                decode_fault = false;
            } break;
            case 0x17:
            {
                // AUIPC
                processU<int64_t>(ins, op_code, rd, simm64);

                bundle->addInstruction(new VanadisPCAddImmInstruction<int64_t>(
                    ins_address, hw_thr, options, rd, simm64));
                decode_fault = false;
            } break;

            case 0x1B:
            {
                processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                output->verbose(
                    CALL_INFO, 16, 0, "-----> decode R-type, func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n",
                    func_code3, func_code7);

                switch ( func_code3 ) {
                case 0x0:
                {
                    // ADDIW?
                    int64_t addiw_imm = 0;
                    processI(ins, op_code, rd, rs1, func_code3, addiw_imm);

                    output->verbose(
                        CALL_INFO, 16, 0, "-------> ADDIW %" PRIu16 " <- %" PRIu16 " + %" PRId64 "\n", rd, rs1,
                        addiw_imm);

                    bundle->addInstruction(new VanadisAddImmInstruction<int32_t>(
                        ins_address, hw_thr, options, rd, rs1, static_cast<int32_t>(addiw_imm)));
                    decode_fault = false;
                } break;
                case 0x1:
                {
                    switch ( func_code7 ) {
                    case 0x0:
                    {
                        // RS2 acts as an immediate
                        // SLLIW (32bit result generated)
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> SLLIW %" PRIu16 " <- %" PRIu16 " << %" PRIu16 " (0x%lx)\n", rd,
                            rs1, rs2, rs2);
                        bundle->addInstruction(
                            new VanadisShiftLeftLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    }
                } break;
                case 0x5:
                {
                    switch ( func_code7 ) {
                    case 0x0:
                    {
                        // RS2 acts as an immediate
                        // SRLIW (32bit result generated)
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> SRLIW %" PRIu16 " <- %" PRIu16 " << %" PRIu16 " (0x%lx)\n", rd,
                            rs1, rs2, rs2);
                        bundle->addInstruction(
                            new VanadisShiftRightLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;

                    } break;
                    case 32:
                    {
                        // RS2 acts as an immediate
                        // SRAIW
                        output->verbose(
                            CALL_INFO, 16, 0, "-------> SRAIW %" PRIu16 " <- %" PRIu16 " << %" PRIu16 " (0xlx)\n", rd,
                            rs1, rs2, rs2);
                        bundle->addInstruction(
                            new VanadisShiftRightArithmeticImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    }
                } break;
                }
            } break;
            case 0x6F:
            {
                // JAL
                processJ<int64_t>(ins, op_code, rd, simm64);
                // Immediate specifies jump in multiples of 2 byts per RISCV spec
                const int64_t jump_to = static_cast<int64_t>(ins_address) + simm64;

                output->verbose(
                    CALL_INFO, 16, 0,
                    "-----> JAL link-reg: %" PRIu16 " / jump-address: 0x%llx + %" PRId64 " = 0x%llx\n", rd, ins_address,
                    simm64, jump_to);

                bundle->addInstruction(new VanadisJumpLinkInstruction(
                    ins_address, hw_thr, options, 4, rd, jump_to, VANADIS_NO_DELAY_SLOT));
                decode_fault = false;
            } break;
            case 0x67:
            {
                // JALR
                processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);

                switch ( func_code3 ) {
                case 0:
                {
                    // TODO - may need to zero bit 1 with an AND microop?
                    bundle->addInstruction(new VanadisJumpRegLinkInstruction(
                        ins_address, hw_thr, options, 4, rd, rs1, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                };
            } break;
            case 0x63:
            {
                // Branch
                processB<int64_t>(ins, op_code, rs1, rs2, func_code, simm64);

                switch ( func_code ) {
                case 0:
                {
                    // BEQ
                    output->verbose(
                        CALL_INFO, 16, 0, "-----> BEQ %" PRIu16 " == %" PRIu16 " / offset: %" PRId64 "\n", rs1, rs2,
                        simm64);
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_EQ, true>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 1:
                {
                    // BNE
                    output->verbose(
                        CALL_INFO, 16, 0,
                        "-----> BNE %" PRIu16 " != %" PRIu16 " / offset: %" PRId64 " (ip+offset %" PRIu64 ")\n", rs1,
                        rs2, simm64, ins_address + simm64);
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_NEQ, true>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 4:
                {
                    // BLT
                    output->verbose(
                        CALL_INFO, 16, 0, "-----> BLT %" PRIu16 " < %" PRIu16 " / offset: %" PRId64 "\n", rs1, rs2,
                        simm64);
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_LT, true>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 5:
                {
                    // BGE
                    output->verbose(
                        CALL_INFO, 16, 0, "-----> BGE %" PRIu16 " >= %" PRIu16 " / offset: %" PRId64 "\n", rs1, rs2,
                        simm64);
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_GTE, true>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 6:
                {
                    // BLTU
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_LT, false>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 7:
                {
                    // BGEU
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_GTE, false>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                };
            } break;
            case 0x73:
            {
                // Syscall/ECALL and EBREAK
                // Control registers

                processI<uint64_t>(ins, op_code, rd, rs1, func_code, uimm64);

                if ( (0 == rd) && (0 == rs1) && (0 == func_code) ) {
                    uint32_t func_code12 = (ins & 0xFFF00000);

                    switch ( func_code12 ) {
                    case 0x0:
                    {
                        output->verbose(CALL_INFO, 16, 0, "------> ECALL/SYSCALL\n");
						bundle->addInstruction(new VanadisFenceInstruction(ins_address, hw_thr, options, VANADIS_LOAD_STORE_FENCE));
                        bundle->addInstruction(new VanadisSysCallInstruction(ins_address, hw_thr, options));
                        decode_fault = false;
                    } break;
                    }
                }
                else {
                    switch ( func_code ) {
                    case 0x1:
                    {
                        // CSRRW
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> CSRRW CSR: ins: 0x%llx / %" PRIu64 " / rd: %" PRIu16 " / rs1: %" PRIu16 "\n", ins_address, uimm64, rd, rs1);

                        // Switch based on the CSR being read
                        switch ( uimm64 ) {
                        case 0x1:
                        {
                            // Pseudoinstructions FSFLAGS; csrrw x0, fflags rs  
                            output->verbose( CALL_INFO, 16, 0, "-----> FSFLAGS: %" PRIu64 " / rd: %" PRIu16 " / rs1: %" PRIu16 "\n", uimm64, rd, rs1);

                            // CSSRW x0, clear the flags
							bundle->addInstruction(new VanadisFPFlagsSetInstruction(ins_address, hw_thr, options, fpflags, rd));
                            // FFLAGS rs, write the flags 
							bundle->addInstruction(new VanadisFPFlagsSetInstruction(ins_address, hw_thr, options, fpflags, rs1));
                            decode_fault = false;
                        } break;
                        default: assert(0);
                        }
                    } break;
                    case 0x2:
                    {
                        // CSRRS
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> CSRRS CSR: ins: 0x%llx / %" PRIu64 " / rd: %" PRIu16 " / rs1: %" PRIu16 "\n", ins_address, uimm64, rd, rs1);
                        // ignore for now?

                        // Switch based on the CSR being read
                        switch ( uimm64 ) {
                        case 0x1:
                        {
                            // Pseudoinstructions FRFLAGS; csrrs rd, fflags x0  
                            output->verbose( CALL_INFO, 16, 0, "-----> FRFLAGS: %" PRIu64 " / rd: %" PRIu16 " / rs1: %" PRIu16 "\n", uimm64, rd, rs1);

                            // CSRRS rd ; put fpflags in rd
							bundle->addInstruction(new VanadisFPFlagsReadInstruction<true, true, true>(ins_address, hw_thr, options, fpflags, rd));
                            // FFLAGS x0 ; zero fp fpflags
							bundle->addInstruction(new VanadisFPFlagsSetInstruction(ins_address, hw_thr, options, fpflags, rs1));

                            decode_fault = false;
                        } break;
                        case 0x2:
                        {
                            // Floating point rounding mode is being read (frrm instruction)
                            output->verbose(
                                CALL_INFO, 16, 0,
                                "-------> Mapping read rounding-mode register.\n");

										bundle->addInstruction(new VanadisFPFlagsReadInstruction<true, false, false>(ins_address, hw_thr, options, fpflags, rd));
                              decode_fault = false;
                        } break;
                        }
                    } break;
                    case 0x6:
                    {
                        // CSRRSI
                        output->verbose(
                            CALL_INFO, 16, 0,
                            "-----> CSRRS CSRRSI: ins: 0x%llx / %" PRIu64 " / reg: %" PRIu16 " / UIMMM: %" PRIu16 "\n", ins_address, uimm64, rd, rs1);

                        if ( 0 == rd ) {
										switch(uimm64) {
									   case 1: {
											// CSR register 1 maps to floating point flags
											bundle->addInstruction(new VanadisFPFlagsSetImmInstruction(ins_address, hw_thr, options, fpflags, static_cast<uint64_t>(rs1)));
      	                        decode_fault = false;
											} break;
										}
                        }
                    } break;
                    }
                }
            } break;
            case 0x3B:
            {
                // 64b integer arithmetic-W
                processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                switch ( func_code3 ) {
                case 0:
                {
                    switch ( func_code7 ) {
                    case 0x0:
                    {
                        // ADDW
                        // TODO - check register ordering
                        bundle->addInstruction(
                            new VanadisAddInstruction<int32_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x1:
                    {
                        // MULW
                        // TODO - check register ordering
                        bundle->addInstruction(
                            new VanadisMultiplyInstruction<int32_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x20:
                    {
                        // SUBW
                        // TODO - check register ordering
                        bundle->addInstruction(new VanadisSubInstruction<int32_t>(
                            ins_address, hw_thr, options, rd, rs1, rs2, true));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 1:
                {
                    switch ( func_code7 ) {
                    case 0x0:
                    {
                        // SLLW
                        // TODO - check register ordering
                        bundle->addInstruction(
                            new VanadisShiftLeftLogicalInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 4:
                {
                    switch ( func_code7 ) {
                    case 0x1:
                    {
                        // DIVW
                        bundle->addInstruction(
                            new VanadisDivideInstruction<int32_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 5:
                {
                    switch ( func_code7 ) {
                    case 0x0:
                    {
                        // SRLW
                        // TODO - check register ordering
                        bundle->addInstruction(
                            new VanadisShiftRightLogicalInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x1:
                    {
                        // DIVUW
                        bundle->addInstruction(
                            new VanadisDivideInstruction<uint32_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x20:
                    {
                        // SRAW
                        // TODO - check register ordering
                        bundle->addInstruction(
                            new VanadisShiftRightArithmeticInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 6:
                {
                    switch ( func_code7 ) {
                    case 0x1:
                    {
                        // REMW
                        output->verbose(
                            CALL_INFO, 16, 0, "----> REMW %" PRIu16 " <- %" PRIu16 " %% %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(
                            new VanadisModuloInstruction<int32_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 7:
                {
                    switch ( func_code7 ) {
                    case 0x1:
                    {
                        // REMUW
                        output->verbose(
                            CALL_INFO, 16, 0, "----> REMUW %" PRIu16 " <- %" PRIu16 " %% %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(
                            new VanadisModuloInstruction<uint32_t>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    }
                }; break;
                };
            } break;
				case 0xF:
			   {
					// Fence operations
					// For now, we conduct a heavy fence
				   // could optimize this to be more efficient
					output->verbose(CALL_INFO, 16, 0, "----> FENCE\n");
					bundle->addInstruction(
	                             	new VanadisFenceInstruction(ins_address, hw_thr, options, VANADIS_LOAD_STORE_FENCE));
               decode_fault = false;
				} break;
            case 0x2F:
            {
                // Atomic operations (A extension)
                processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                const bool perform_aq = func_code & 0x2;
                const bool perform_rl = func_code & 0x1;

                uint32_t op_width = 0;

                switch(func_code3) {
                case 0x2:
                    op_width = 4;
                    break;
                case 0x3:
                    op_width = 8;
                    break;
                default:
                    op_width = 0;
                    break;
                }

                // need to mask out AQ and RL bits, these are used for deciding on fence operations
                switch ( (func_code7 & 0x7C) ) {
                case 0x4:
                {
                    if(LIKELY(op_width != 0)) {
                        // AMO.SWAP
                        output->verbose(CALL_INFO, 16, 0, "-----> AMOSWAP 0x%llx / thr: %" PRIu32 " / %" PRIu16 " <- memory[ %" PRIu16 " ] <- %" PRIu16 " / width: %" PRIu32 " / aq: %s / rl: %s\n",
                            ins_address, hw_thr, rd, rs1, rs2, op_width, perform_aq ?  "yes" : "no", perform_rl ? "yes" : "no");

                        if(LIKELY(perform_aq)) {
                            bundle->addInstruction(new VanadisFenceInstruction(ins_address, hw_thr, options, VANADIS_LOAD_STORE_FENCE));
                        }

                        // implement a micro-op sequence for AMOSWAP (yuck)
                        bundle->addInstruction(new VanadisLoadInstruction(
                                    ins_address, hw_thr, options, rs1, 0, /* place in r32 */ 32, op_width, 
                                    true, MEM_TRANSACTION_LLSC_LOAD,
                                    LOAD_INT_REGISTER));

                        // 0 is a success, 1 is a failure 
                        bundle->addInstruction(new VanadisStoreConditionalInstruction(
                                ins_address, hw_thr, options, rs1, 0, rs2, /* place in r33 */ 33, op_width, 
                                STORE_INT_REGISTER, 0, 1));

                        // conditionally copy the loaded value into rd IF the store-conditional was successful
                        // copy r32 into rd if r33 == 0
                        bundle->addInstruction(new VanadisConditionalMoveImmInstruction<int64_t, int64_t, REG_COMPARE_EQ>(
                                ins_address, hw_thr, options, rd, 32, 33, 0
                        ));

                        // branch back to this instruction address IF r32 == 1 (which means the store failed)
                        bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<int64_t, REG_COMPARE_EQ>(
                            ins_address, hw_thr, options, 4, 32, 1, 
                            /* offset is 0 so we replay this instruction */ 0, VANADIS_NO_DELAY_SLOT
                        ));

                        if(LIKELY(perform_rl)) {
                            bundle->addInstruction(new VanadisFenceInstruction(ins_address, hw_thr, options, VANADIS_LOAD_STORE_FENCE));
                        }

                        decode_fault = false;
                    }
                } break;
                case 0x8:
                {
                    if ( rs2 == 0 ) {
                        // LR.?.AQ.RL
                        if(LIKELY(op_width != 0)) {
                            output->verbose(
                                CALL_INFO, 16, 0, "-----> LR 0x%llx / thr: %" PRIu32 " / (LLSC_LOAD) %" PRIu16 " <- memory[ %" PRIu16 " ] / width: %" PRIu32 " / aq: %s / rl: %s\n",
                                    ins_address, hw_thr, rd, rs1, op_width, perform_aq ?  "yes" : "no", perform_rl ? "yes" : "no");

                            if(LIKELY(perform_aq)) {
                                bundle->addInstruction(new VanadisFenceInstruction(ins_address, hw_thr, options, VANADIS_LOAD_STORE_FENCE));
                            }

                            bundle->addInstruction(new VanadisLoadInstruction(
                                ins_address, hw_thr, options, rs1, 0, rd, op_width, true, MEM_TRANSACTION_LLSC_LOAD,
                                LOAD_INT_REGISTER));

                            if(LIKELY(perform_rl)) {
                                bundle->addInstruction(new VanadisFenceInstruction(ins_address, hw_thr, options, VANADIS_LOAD_STORE_FENCE));
                            }

                            decode_fault = false;
                        }
                    }
                    else {
                        // ?
                    }
                } break;
                case 0xC:
                {
                    if(LIKELY(op_width != 0)) {
                        output->verbose(
                            CALL_INFO, 16, 0,
                            "-----> SC 0x%llx / thr: %" PRIu32 " / (LLSC_STORE) %" PRIu16 " -> memory[ %" PRIu16 " ] / result: %" PRIu16 " / width: %" PRIu32 " / aq: %s / rl: %s\n",
                            ins_address, hw_thr, rs2, rs1, rd, op_width, perform_aq ?  "yes" : "no", perform_rl ? "yes" : "no");

                        if(LIKELY(perform_aq)) {
                            bundle->addInstruction(new VanadisFenceInstruction(ins_address, hw_thr, options, VANADIS_LOAD_STORE_FENCE));
                        }

                        bundle->addInstruction(new VanadisStoreConditionalInstruction(
                            ins_address, hw_thr, options, rs1, 0, rs2, rd, op_width, STORE_INT_REGISTER, 0, 1));

                        if(LIKELY(perform_rl)) {
                            bundle->addInstruction(new VanadisFenceInstruction(ins_address, hw_thr, options, VANADIS_LOAD_STORE_FENCE));
                        }

                        decode_fault = false;
                    }
                } break;
                }
            } break;
            case 0x27:
            {
                // Floating point store
					 processS<int64_t>(ins, op_code, rs1, rs2, func_code3, simm64);

					 output->verbose(CALL_INFO, 16, 0, "---> STORE-FP func_code3=%" PRIu32 " imm=%" PRId64 " / rs1: %" PRIu16 " / rs2: %" PRIu16 "\n",
						func_code3, simm64, rs1, rs2);

					switch(func_code3) {
					case 0x2:
						{
							output->verbose(CALL_INFO, 16, 0, "-------> FSW imm=%" PRId64 " / rs1: %" PRIu16 " / rs2: %" PRIu16 "\n", simm64, rs1, rs2);
							bundle->addInstruction(new VanadisStoreInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rs2, 4, MEM_TRANSACTION_NONE, STORE_FP_REGISTER));
                    	decode_fault = false;
						} break;
					case 0x3:
						{
							output->verbose(CALL_INFO, 16, 0, "-------> FSD imm=%" PRId64 " / rs1: %" PRIu16 " / rs2: %" PRIu16 "\n", simm64, rs1, rs2);
							bundle->addInstruction(new VanadisStoreInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rs2, 8, MEM_TRANSACTION_NONE, STORE_FP_REGISTER));
                    	decode_fault = false;
						} break;
					}
            } break;
            case 0x53:
            {
                // floating point arithmetic
                processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                output->verbose(
                    CALL_INFO, 16, 0, "---> func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n", func_code3, func_code7);

                switch ( func_code7 ) {
                case 44:
                {
                    //FSQRT.S
                } break;
                case 45:
                {
                    //FSQRT.D
                    output->verbose(
                        CALL_INFO, 16, 0, "-----> FSQRT.D %" PRIu16 " <- %" PRIu16 "\n", rd, rs1);
                    bundle->addInstruction(
                        new VanadisFPSquareRootInstruction<double>(ins_address, hw_thr, options, fpflags, rd, rs1));
                    decode_fault = false;
                } break;
                case 0:
                {
                    // FADD.S
                    output->verbose(
                        CALL_INFO, 16, 0, "-----> FADD.S %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n", rd, rs1, rs2);
                    bundle->addInstruction(
                        new VanadisFPAddInstruction<float>(ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                    decode_fault = false;
                } break;
                case 0x1:
                {
                    // FADD.D
                    output->verbose(
                        CALL_INFO, 16, 0, "-----> FADD.D %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n", rd, rs1, rs2);
                    bundle->addInstruction(
                        new VanadisFPAddInstruction<double>(ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                    decode_fault = false;
                } break;
                case 4:
                {
                    // FSUB.S
                    output->verbose(
                        CALL_INFO, 16, 0, "-----> FSUB.S %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n", rd, rs1, rs2);
                    bundle->addInstruction(
                        new VanadisFPSubInstruction<float>(ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                    decode_fault = false;
                } break;
                case 5:
                {
                    // FSUB.D
                    output->verbose(
                        CALL_INFO, 16, 0, "-----> FSUB.D %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n", rd, rs1, rs2);
                    bundle->addInstruction(
                        new VanadisFPSubInstruction<double>(ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                    decode_fault = false;
                } break;
					 case 8:
					 {
							// FMUL.S
                    output->verbose(
                        CALL_INFO, 16, 0, "-----> FMUL.S %" PRIu16 " <- %" PRIu16 " * %" PRIu16 "\n", rd, rs1, rs2);
                    bundle->addInstruction(
                        new VanadisFPMultiplyInstruction<float>(ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                    decode_fault = false;
					 } break;
                case 9:
                {
                    // FMUL.D
                    output->verbose(
                        CALL_INFO, 16, 0, "-----> FMUL.D %" PRIu16 " <- %" PRIu16 " * %" PRIu16 "\n", rd, rs1, rs2);
                    bundle->addInstruction(
                        new VanadisFPMultiplyInstruction<double>(ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                    decode_fault = false;
                } break;
                case 16:
                {
                    switch ( func_code3 ) {
                    case 0:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FSGNJ.S %" PRIu16 " <- %" PRIu16 " / %" PRIu16 "\n", rd, rs1,
                            rs2);
                        bundle->addInstruction(
                            new VanadisFPSignLogicInstruction<float, VanadisFPSignLogicOperation::SIGN_COPY>(
                                ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FSGNJN.S %" PRIu16 " <- %" PRIu16 " / %" PRIu16 "\n", rd, rs1,
                            rs2);
                        bundle->addInstruction(
                            new VanadisFPSignLogicInstruction<float, VanadisFPSignLogicOperation::SIGN_NEG>(
                                ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 2:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FSGNJX.S %" PRIu16 " <- %" PRIu16 " / %" PRIu16 "\n", rd, rs1,
                            rs2);
                        bundle->addInstruction(
                            new VanadisFPSignLogicInstruction<float, VanadisFPSignLogicOperation::SIGN_XOR>(
                                ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    }
                } break;
                case 17:
                {
                    switch ( func_code3 ) {
                    case 0:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FSGNJ.D %" PRIu16 " <- %" PRIu16 " / %" PRIu16 "\n", rd, rs1,
                            rs2);
                        bundle->addInstruction(
                            new VanadisFPSignLogicInstruction<double, VanadisFPSignLogicOperation::SIGN_COPY>(
                                ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FSGNJN.D %" PRIu16 " <- %" PRIu16 " / %" PRIu16 "\n", rd, rs1,
                            rs2);
                        bundle->addInstruction(
                            new VanadisFPSignLogicInstruction<double, VanadisFPSignLogicOperation::SIGN_NEG>(
                                ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 2:
                    {
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FSGNJX.D %" PRIu16 " <- %" PRIu16 " / %" PRIu16 "\n", rd, rs1,
                            rs2);
                        bundle->addInstruction(
                            new VanadisFPSignLogicInstruction<double, VanadisFPSignLogicOperation::SIGN_XOR>(
                                ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    }
                } break;
					 case 12:
					 {
                    output->verbose(
                        CALL_INFO, 16, 0, "---> FDIV.S %" PRIu16 " <- %" PRIu16 " / %" PRIu16 "\n", rd, rs1, rs2);
                    bundle->addInstruction(new VanadisFPDivideInstruction<float>(
                        ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                    decode_fault = false;
					 } break;
                case 0xD:
                {
                    output->verbose(
                        CALL_INFO, 16, 0, "---> FDIV.D %" PRIu16 " <- %" PRIu16 " / %" PRIu16 "\n", rd, rs1, rs2);
                    bundle->addInstruction(new VanadisFPDivideInstruction<double>(
                        ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                    decode_fault = false;
                } break;
                case 21:
                {
                    output->verbose(
                        CALL_INFO, 16, 0, "---> FMIN.D %" PRIu16 " <- %" PRIu16 " / %" PRIu16 "\n", rd, rs1, rs2);
                    bundle->addInstruction(new VanadisFPMinimumInstruction<double>(
                        ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                    decode_fault = false;
                } break;
					 case 33:
					 {
                    processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                    output->verbose(
                        CALL_INFO, 16, 0, "---> func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n", func_code3,
                        func_code7);

                    // rs2 dictates the signed nature of the operands
						  // we use FP convert here because these are FP to FP regiser modifications
                    switch ( rs2 ) {
                    case 0:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.D.S %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisFPConvertInstruction<float, double>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.S.D %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisFPConvertInstruction<double, float>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    } break;
                    }

					} break;
                case 104:
                {
                    processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                    output->verbose(
                        CALL_INFO, 16, 0, "---> func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n", func_code3,
                        func_code7);

                    // rs2 dictates the signed nature of the operands
                    switch ( rs2 ) {
                    case 0:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.S.W %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisGPR2FPInstruction<int32_t, float>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.S.WU %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisGPR2FPInstruction<uint32_t, float>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    } break;
                    }
                } break;
                case 97:
                {
                    processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);
                    output->verbose(
                        CALL_INFO, 16, 0, "---> func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n", func_code3,
                        func_code7);

                    // rs2 dictates the signed nature of the operands
                    switch ( rs2 ) {
                    case 0:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.W.D %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisGPR2FPInstruction<double, int32_t>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.WU.D %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisGPR2FPInstruction<double, uint32_t>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    } break;
                    case 2:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.L.D %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisGPR2FPInstruction<double, int64_t>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    }
                    case 3:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.LU.D %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisGPR2FPInstruction<double, uint64_t>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    }
                    }
                } break;
                case 105:
                {
                    processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                    output->verbose(
                        CALL_INFO, 16, 0, "---> func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n", func_code3,
                        func_code7);

                    // rs2 dictates the signed nature of the operands
                    switch ( rs2 ) {
                    case 0:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.D.W %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisGPR2FPInstruction<int32_t, double>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    } break;
                    case 1:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.D.WU %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisGPR2FPInstruction<uint32_t, double>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    } break;
                    case 2:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.D.L %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisGPR2FPInstruction<int64_t, double>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    }
                    case 3:
                    {
                        output->verbose(CALL_INFO, 16, 0, "-----> FCVT.D.LU %" PRIu16 " <- %" PRIu16 "\n", rs1, rd);
                        bundle->addInstruction(
                            new VanadisGPR2FPInstruction<uint64_t, double>(ins_address, hw_thr, options, fpflags, rd, rs1));
                        decode_fault = false;
                    }
                    }
                } break;
				 case 112:
				 {
                    processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                    output->verbose(
                        CALL_INFO, 16, 0, "---> func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n", func_code3,
                        func_code7);

                    // rs2 dictates the signed nature of the operands
                    switch ( rs2 ) {
                    case 0:
                    {
						switch(func_code3) {
							case 0: {
	                            output->verbose(CALL_INFO, 16, 0, "-----> FMV.X.W %" PRIu16 " <- %" PRIu16 "\n", rd, rs1);
                                bundle->addInstruction(
                                new VanadisFP2GPRInstruction<int32_t, int32_t>(ins_address, hw_thr, options, fpflags, rd, rs1));
         	                    decode_fault = false;
							} break;
						}
                    } break;
                   }
			    } break;
				 case 120:
				 {
                    processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                    output->verbose(
                        CALL_INFO, 16, 0, "---> func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n", func_code3,
                        func_code7);

                    // rs2 dictates the signed nature of the operands
                    switch ( rs2 ) {
                    case 0:
                    {
								switch(func_code3) {
								case 0: {
	                        output->verbose(CALL_INFO, 16, 0, "-----> FMV.W.X %" PRIu16 " <- %" PRIu16 "\n", rd, rs1);
   	                     bundle->addInstruction(
      	                      new VanadisGPR2FPInstruction<int32_t, int32_t>(ins_address, hw_thr, options, fpflags, rd, rs1));
         	               decode_fault = false;
								} break;
								}
                    } break;
                   }
			    } break;
                case 0x50:
                {
                    switch ( func_code3 ) {
                    case 0x0:
                    {
                        // FLE.S
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FLE.S %" PRIu16 " <- %" PRIu16 " <= %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(new VanadisFPSetRegCompareInstruction<REG_COMPARE_LTE, float>(
                            ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x1:
                    {
                        // FLT.S
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FLT.S %" PRIu16 " <- %" PRIu16 " <= %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(new VanadisFPSetRegCompareInstruction<REG_COMPARE_LT, float>(
                            ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    }
                } break;
                case 0x51:
                {
                    switch ( func_code3 ) {
                    case 0x0:
                    {
                        // FLE.D
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FLE.D %" PRIu16 " <- %" PRIu16 " <= %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(new VanadisFPSetRegCompareInstruction<REG_COMPARE_LTE, double>(
                            ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x1:
                    {
                        // FLT.D
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FLT.D %" PRIu16 " <- %" PRIu16 " < %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(new VanadisFPSetRegCompareInstruction<REG_COMPARE_LT, double>(
                            ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x2:
                    {
                        // FEQ.D
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FEQ.D %" PRIu16 " <- %" PRIu16 " < %" PRIu16 "\n", rd, rs1, rs2);
                        bundle->addInstruction(new VanadisFPSetRegCompareInstruction<REG_COMPARE_EQ, double>(
                            ins_address, hw_thr, options, fpflags, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    }
                } break;
                case 0x70:
                {
                     // fmv.x.s
                    processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                    if ( 0 == rs2 ) {
                        if ( 0 == func_code3 ) {
                            output->verbose(CALL_INFO, 16, 0, "-----> FMV.X.S %" PRIu16 " <- %" PRIu16 "\n", rd, rs1);
                            bundle->addInstruction(
                                new VanadisFP2GPRInstruction<int64_t, int64_t>(ins_address, hw_thr, options, fpflags, rd, rs1));
                            decode_fault = false;
                        }
                    }
                } break;
                case 0x79:
                {
                    processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                    if ( 0 == rs2 ) {
                        if ( 0 == func_code3 ) {
                            output->verbose(CALL_INFO, 16, 0, "-----> FMV.D.X %" PRIu16 " <- %" PRIu16 "\n", rd, rs1);
                            bundle->addInstruction(
                                new VanadisGPR2FPInstruction<int64_t, int64_t>(ins_address, hw_thr, options, fpflags, rd, rs1));
                            decode_fault = false;
                        }
                    }
                } break;
                }
            } break;
            case 0x43:
            {
                uint32_t fmt;
                processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);
                fmt = func_code7 & 0x3;
                rs3 = func_code7 >> 2;

                switch( fmt ) {
                    case 1:
                    {
                        // FMADD.D
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FMADD.D %" PRIu16 " <- ( %" PRIu16 " *  %" PRIu16 " ) + %" PRIu16 "\n", rd, rs1, rs2, rs3);
                        bundle->addInstruction(
                            new VanadisFPFusedMultiplyAddInstruction<double>(ins_address, hw_thr, options, fpflags, rd, rs1, rs2, rs3 ));
                        decode_fault = false;
                    } break;
                }
            } break;
            case 0x47:
            {
                // FMSUB
                uint32_t fmt;
                processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);
                fmt = func_code7 & 0x3;
                rs3 = func_code7 >> 2;

                switch( fmt ) {
                    case 1:
                    {
                        // FMSUB.D
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FMSUB.D %" PRIu16 " <- ( %" PRIu16 " *  %" PRIu16 " ) -  %" PRIu16 "\n", rd, rs1, rs2, rs3);
                        bundle->addInstruction(
                            new VanadisFPFusedMultiplySubInstruction<double,false>(ins_address, hw_thr, options, fpflags, rd, rs1, rs2, rs3 ));
                        decode_fault = false;
                    } break;
                }
            } break;
            case 0x4B:
            {
                uint32_t fmt;
                processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);
                fmt = func_code7 & 0x3;
                rs3 = func_code7 >> 2;

                output->verbose(CALL_INFO, 16, 0, "-----> fmt: %" PRIu32 " rs3: %" PRIu32 "\n", fmt, rs3);

                switch( fmt ) {
                    case 1:
                    {
                        // FNMSUB.D
                        output->verbose(
                            CALL_INFO, 16, 0, "-----> FNMSUB.D %" PRIu16 " <- ( - %" PRIu16 " *  %" PRIu16 " ) -  %" PRIu16 "\n", rd, rs1, rs2, rs3);
                        bundle->addInstruction(
                            new VanadisFPFusedMultiplySubInstruction<double,true>(ins_address, hw_thr, options, fpflags, rd, rs1, rs2, rs3 ));
                        decode_fault = false;
                    } break;
                }
            } break;
            case 0x4F:
            {
                // FNMADD
            } break;
            default:
            {
                // op_code did not match an expected value
            } break;
            }
        }
        else {
            const uint32_t c_op_code = ins & 0x3;

            // this bundle only increments the PC by 2, not 4 in 32bit decodes
            bundle->setPCIncrement(2);

            output->verbose(CALL_INFO, 16, 0, "-----> RVC op_code: %" PRIu32 "\n", c_op_code);

            switch ( c_op_code ) {
            case 0x0:
            {
                const uint32_t c_func_code = ins & 0xE000;

                switch ( c_func_code ) {
                case 0x0:
                {
                    if ( (ins & 0xFFFF) == 0 ) {
                        // This is an illegal instruction (all zeroes)
                    }
                    else {
                        // ADDI4SPN
                        uint16_t rvc_rd = expand_rvc_int_register(extract_rs2_rvc(ins));

                        uint32_t imm_3  = (ins & 0x20) >> 2;
                        uint32_t imm_2  = (ins & 0x40) >> 4;
                        uint32_t imm_96 = (ins & 0x780) >> 1;
                        uint32_t imm_54 = (ins & 0x1800) >> 7;

                        uint32_t imm = (imm_2 | imm_3 | imm_54 | imm_96);

                        output->verbose(
                            CALL_INFO, 16, 0, "-------> RVC ADDI4SPN %" PRIu16 " <- r2 (stack-ptr) + %" PRIu32 "\n",
                            rvc_rd, imm);

                        bundle->addInstruction(
                            new VanadisAddImmInstruction<int64_t>(
                                ins_address, hw_thr, options, rvc_rd, 2, imm));
                        decode_fault = false;
                    }
                } break;
                case 0x2000:
                {
                    // FLD
                    uint16_t rvc_rd  = expand_rvc_int_register(extract_rs2_rvc(ins));
                    uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                    uint32_t imm_53 = (ins & 0x1C00) >> 7;
                    uint32_t imm_76 = (ins & 0x60) << 1;

                    uint32_t imm = (imm_53 | imm_76);

                    output->verbose(
                        CALL_INFO, 16, 0, "-------> RVC FLD %" PRIu16 " <- memory[ %" PRIu16 " + %" PRIu32 " ]\n",
                        rvc_rd, rvc_rs1, imm);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rvc_rs1, imm, rvc_rd, 8, true, MEM_TRANSACTION_NONE,
                        LOAD_FP_REGISTER));
                    decode_fault = false;
                } break;
                case 0x4000:
                {
                    // LW
                    uint64_t imm_address = extract_uimm_rcv_t2(ins);
                    uint16_t rvc_rd      = expand_rvc_int_register(extract_rs2_rvc(ins));
                    uint16_t rvc_rs1     = expand_rvc_int_register(extract_rs1_rvc(ins));

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC LW rd=%" PRIu16 " <- rs1=%" PRIu16 " + imm=%" PRIu64 "\n", rvc_rd,
                        rvc_rs1, imm_address);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rvc_rs1, imm_address, rvc_rd, 4, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 0x6000:
                {
                    // LD
                    uint64_t imm_address = extract_uimm_rvc_t1(ins);
                    uint16_t rvc_rd      = expand_rvc_int_register(extract_rs2_rvc(ins));
                    uint16_t rvc_rs1     = expand_rvc_int_register(extract_rs1_rvc(ins));

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC LD rd=%" PRIu16 " <- rs1=%" PRIu16 " + imm=%" PRIu64 "\n", rvc_rd,
                        rvc_rs1, imm_address);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rvc_rs1, imm_address, rvc_rd, 8, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 0x8000:
                {
                    // Reserved, this is a decode fault
                } break;
                case 0xA000:
                {
                    // FSD
                    uint16_t rvc_rs2  = expand_rvc_int_register(extract_rs2_rvc(ins));
                    uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                    uint32_t imm_53 = (ins & 0x1C00) >> 7;
                    uint32_t imm_76 = (ins & 0x60) << 1;

                    uint32_t imm = (imm_53 | imm_76);

                    output->verbose(
                        CALL_INFO, 16, 0, "-------> RVC FSD %" PRIu16 " -> memory[ %" PRIu16 " + %" PRIu32 " ]\n",
                        rvc_rs2, rvc_rs1, imm);

                    bundle->addInstruction(new VanadisStoreInstruction(
                        ins_address, hw_thr, options, rvc_rs1, imm, rvc_rs2, 8, MEM_TRANSACTION_NONE,
                        STORE_FP_REGISTER));
                    decode_fault = false;
                } break;
                case 0xC000:
                {
                    // SW
                    uint16_t rvc_rs2 = expand_rvc_int_register(extract_rs2_rvc(ins));
                    uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                    uint32_t offset_53 = (ins & 0x1C00) >> 7;
                    uint32_t offset_6  = (ins & 0x20) << 1;
                    uint32_t offset_2  = (ins & 0x40) >> 4;

                    uint64_t offset = (offset_2 | offset_6 | offset_53);

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC SW %" PRIu16 " -> %" PRIu16 " + %" PRIu64 "\n", rvc_rs2, rvc_rs1,
                        offset);

                    bundle->addInstruction(new VanadisStoreInstruction(
                        ins_address, hw_thr, options, rvc_rs1, offset, rvc_rs2, 4, MEM_TRANSACTION_NONE,
                        STORE_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 0xE000:
                {
                    // SD
                    uint16_t rvc_rs2 = expand_rvc_int_register(extract_rs2_rvc(ins));
                    uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                    uint32_t offset_53 = (ins & 0x1C00) >> 7;
                    uint32_t offset_6  = (ins & 0x20) << 1;
                    uint32_t offset_7  = (ins & 0x40) << 1;

                    uint64_t offset = (offset_7 | offset_6 | offset_53);

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC SD %" PRIu16 " -> %" PRIu16 " + %" PRIu64 "\n", rvc_rs2, rvc_rs1,
                        offset);

                    bundle->addInstruction(new VanadisStoreInstruction(
                        ins_address, hw_thr, options, rvc_rs1, offset, rvc_rs2, 8, MEM_TRANSACTION_NONE,
                        STORE_INT_REGISTER));
                    decode_fault = false;
                } break;
                }
            } break;
            case 0x1:
            {
                const uint32_t c_func_code = ins & 0xE000;

                switch ( c_func_code ) {
                case 0x0:
                {
                    // NOP // ADDI
                    uint16_t rvc_rs1 = static_cast<uint16_t>((ins & 0xF80) >> 7);

                    if ( 0 == rvc_rs1 ) {
                        // This is a no-op?
                        output->verbose(CALL_INFO, 16, 0, "-----> creates a no-op.\n");
                        bundle->addInstruction(
                            new VanadisAddImmInstruction<int64_t>(
                                ins_address, hw_thr, options, 0, 0, 0));
                        decode_fault = false;
                    }
                    else {
                        uint32_t imm_40 = (ins & 0x7C) >> 2;
                        uint32_t imm_5  = (ins & 0x1000) >> 7;

                        int64_t imm = (imm_40 | imm_5);

                        if ( imm_5 != 0 ) { imm |= 0xFFFFFFFFFFFFFFC0; }

                        if ( imm_5 == 0 ) {
                            // Tehcnicall this is a HINT, now what do we do?
                        }

                        output->verbose(
                            CALL_INFO, 16, 0, "-----> RVC ADDI %" PRIu16 " = %" PRIu16 " + %" PRId64 "\n", rvc_rs1,
                            rvc_rs1, imm);

                        bundle->addInstruction(
                            new VanadisAddImmInstruction<int64_t>(
                                ins_address, hw_thr, options, rvc_rs1, rvc_rs1, imm));
                        decode_fault = false;
                    }
                } break;
                case 0x2000:
                {
                    // ADDIW
                    uint16_t rd     = static_cast<uint16_t>((ins & 0xF80) >> 7);
                    uint32_t imm_40 = (ins & 0x7C) >> 2;
                    uint32_t imm_5  = (ins & 0x1000) >> 7;

                    int32_t imm = (imm_40 | imm_5);

                    if ( imm_5 != 0 ) { imm |= 0xFFFFFFC0; }

                    output->verbose(CALL_INFO, 16, 0, "-----> RVC ADDIW  reg: %" PRIu16 ", imm=%" PRId32 "\n", rd, imm);

                    // This really is a W-clipping instruction, so make INT32
                    bundle->addInstruction(new VanadisAddImmInstruction<int32_t>(
                        ins_address, hw_thr, options, rd, rd, static_cast<int32_t>(imm)));
                    decode_fault = false;
                } break;
                case 0x4000:
                {
                    // LI
                    uint16_t rd     = static_cast<uint16_t>((ins & 0xF80) >> 7);
                    uint32_t imm_40 = (ins & 0x7C) >> 2;
                    uint32_t imm_5  = (ins & 0x1000) >> 7;

                    int64_t imm = (imm_40 | imm_5);

                    if ( imm_5 != 0 ) { imm |= 0xFFFFFFFFFFFFFFC0; }

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC load imediate (LI) reg: %" PRIu16 ", imm=%" PRId64 "\n", rd, imm);

                    bundle->addInstruction(
                        new VanadisSetRegisterInstruction<int64_t>(
                            ins_address, hw_thr, options, rd, imm));
                    decode_fault = false;
                } break;
                case 0x6000:
                {
                    // ADDI16SP / LUI
                    uint16_t rd = static_cast<uint16_t>((ins & 0xF80) >> 7);

                    if ( rd != 2 ) {
                        // LUI
                        uint32_t imm_1612 = (ins & 0x7C) << 10;
                        uint32_t imm_17   = (ins & 0x1000) << 5;

                        int32_t imm = (imm_17 | imm_1612);

								if(imm_17 != 0) {
									imm = imm | 0xFFFC0000;
								}

                        output->verbose(
                            CALL_INFO, 16, 0,
                            "-----> RVC load imediate (LUI) reg: %" PRIu16 ", imm=%" PRId32 " (0x%lx)\n", rd, imm,
                            imm);

                        bundle->addInstruction(
                            new VanadisSetRegisterInstruction<int32_t>(
                                ins_address, hw_thr, options, rd, imm));
                        decode_fault = false;
                    }
                    else {
                        // ADDI16SP
                        output->verbose(CALL_INFO, 16, 0, "----> RVC ADDI16SP\n");

                        const uint32_t imm_5  = (ins & 0x4) << 3;
                        const uint32_t imm_87 = (ins & 0x18) << 4;
                        const uint32_t imm_6  = (ins & 0x20) << 1;
                        const uint32_t imm_4  = (ins & 0x40) >> 2;
                        const uint32_t imm_9  = (ins & 0x1000) >> 3;

                        int64_t imm = imm_5 | imm_87 | imm_6 | imm_4 | imm_9;

                        if ( imm_9 != 0 ) { imm |= 0xFFFFFFFFFFFFFC00; }

                        output->verbose(CALL_INFO, 16, 0, "-----> RVC ADDI16SP r2 <- %" PRId64 "\n", imm);

                        bundle->addInstruction(
                            new VanadisAddImmInstruction<int64_t>(
                                ins_address, hw_thr, options, 2, 2, imm));
                        decode_fault = false;
                    }
                } break;
                case 0x8000:
                {
                    const uint32_t c_func2_code = ins & 0xC00;

                    output->verbose(CALL_INFO, 16, 0, "------> RCV arith code: %" PRIu32 "\n", c_func2_code);

                    switch ( c_func2_code ) {
                    case 0x0:
                    {
                        // SRLI
                        uint32_t shamt_5  = (ins & 0x1000) >> 7;
                        uint32_t shamt_40 = (ins & 0x7C) >> 2;

                        uint64_t shift_by = shamt_5 | shamt_40;

                        uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                        output->verbose(
                            CALL_INFO, 16, 0, "--------> RVC SRLI %" PRIu16 " = %" PRIu16 " >> %" PRIu64 " (0x%llx)\n",
                            rvc_rs1, rvc_rs1, shift_by, shift_by);
                        bundle->addInstruction(
                            new VanadisShiftRightLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rvc_rs1, rvc_rs1, shift_by));
                        decode_fault = false;
                    } break;
                    case 0x400:
                    {
                        // SRAI
                        uint32_t shamt_5  = (ins & 0x1000) >> 7;
                        uint32_t shamt_40 = (ins & 0x7C) >> 2;

                        uint64_t shift_by = shamt_5 | shamt_40;

                        uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                        output->verbose(
                            CALL_INFO, 16, 0, "--------> RVC SRAI %" PRIu16 " = %" PRIu16 " >> %" PRIu64 " (0x%llx)\n",
                            rvc_rs1, rvc_rs1, shift_by, shift_by);
                        bundle->addInstruction(
                            new VanadisShiftRightArithmeticImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rvc_rs1, rvc_rs1, shift_by));
                        decode_fault = false;
                    } break;
                    case 0x800:
                    {
                        // ANDI
                        uint32_t imm_5  = (ins & 0x1000) >> 7;
                        uint32_t imm_40 = (ins & 0x7C) >> 2;

                        uint64_t imm = imm_5 | imm_40;

                        if ( imm_5 != 0 ) { imm |= 0xFFFFFFFFFFFFFFC0; }

                        uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                        output->verbose(
                            CALL_INFO, 16, 0, "--------> RVC ANDI %" PRIu16 " = %" PRIu16 " & %" PRIu64 " (0x%llx)\n",
                            rvc_rs1, rvc_rs1, imm, imm);

                        bundle->addInstruction(
                            new VanadisAndImmInstruction(ins_address, hw_thr, options, rvc_rs1, rvc_rs1, imm));
                        decode_fault = false;
                    } break;
                    case 0xC00:
                    {
                        // Arith
                        uint16_t rvc_rs2 = expand_rvc_int_register(extract_rs2_rvc(ins));
                        uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                        switch ( ins & 0x1000 ) {
                        case 0x0:
                        {
                            output->verbose(CALL_INFO, 16, 0, "------> RVC arith check zero family\n");

                            switch ( ins & 0x60 ) {
                            case 0x0:
                            {
                                // SUB
                                output->verbose(
                                    CALL_INFO, 16, 0, "--------> RVC SUB %" PRIu16 " <- %" PRIu16 " - %" PRIu16 "\n",
                                    rvc_rs1, rvc_rs1, rvc_rs2);
                                bundle->addInstruction(
                                    new VanadisSubInstruction<int64_t>(
                                        ins_address, hw_thr, options, rvc_rs1, rvc_rs1, rvc_rs2, true));
                                decode_fault = false;
                            } break;
                            case 0x20:
                            {
                                // XOR
                                output->verbose(
                                    CALL_INFO, 16, 0, "--------> RVC XOR %" PRIu16 " <- %" PRIu16 " ^ %" PRIu16 "\n",
                                    rvc_rs1, rvc_rs1, rvc_rs2);
                                bundle->addInstruction(
                                    new VanadisXorInstruction(ins_address, hw_thr, options, rvc_rs1, rvc_rs1, rvc_rs2));
                                decode_fault = false;
                            } break;
                            case 0x40:
                            {
                                // OR
                                output->verbose(
                                    CALL_INFO, 16, 0, "--------> RVC OR %" PRIu16 " <- %" PRIu16 " | %" PRIu16 "\n",
                                    rvc_rs1, rvc_rs1, rvc_rs2);
                                bundle->addInstruction(
                                    new VanadisOrInstruction(ins_address, hw_thr, options, rvc_rs1, rvc_rs1, rvc_rs2));
                                decode_fault = false;
                            } break;
                            case 0x60:
                            {
                                // AND
                                output->verbose(
                                    CALL_INFO, 16, 0, "--------> RVC AND %" PRIu16 " <- %" PRIu16 " & %" PRIu16 "\n",
                                    rvc_rs1, rvc_rs1, rvc_rs2);
                                bundle->addInstruction(
                                    new VanadisAndInstruction(ins_address, hw_thr, options, rvc_rs1, rvc_rs1, rvc_rs2));
                                decode_fault = false;
                            } break;
                            }
                        } break;
                        case 0x1000:
                        {
                            output->verbose(CALL_INFO, 16, 0, "------> RVC arith check one family\n");

                            switch ( ins & 0x60 ) {
                            case 0x0:
                            {
                                // SUBW
                                uint16_t rvc_rs2 = expand_rvc_int_register(extract_rs2_rvc(ins));
                                uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                                output->verbose(
                                    CALL_INFO, 16, 0, "------> RVC SUBW %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n",
                                    rvc_rs1, rvc_rs1, rvc_rs2);

                                bundle->addInstruction(
                                    new VanadisSubInstruction<int32_t>(
                                        ins_address, hw_thr, options, rvc_rs1, rvc_rs1, rvc_rs2, false));
                                decode_fault = false;

                            } break;
                            case 0x20:
                            {
                                // ADDW
                                uint16_t rvc_rs2 = expand_rvc_int_register(extract_rs2_rvc(ins));
                                uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                                output->verbose(
                                    CALL_INFO, 16, 0, "------> RVC ADDW %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n",
                                    rvc_rs1, rvc_rs1, rvc_rs2);

                                bundle->addInstruction(
                                    new VanadisAddInstruction<int32_t>(
                                        ins_address, hw_thr, options, rvc_rs1, rvc_rs1, rvc_rs2));
                                decode_fault = false;

                            } break;
                            case 0x40:
                            {
                                // Reserved, not used
                            } break;
                            case 0x60:
                            {
                                // Reserved, not used
                            } break;
                            }
                        } break;
                        }
                    } break;
                    }
                } break;
                case 0xA000:
                {
                    // JUMP
                    const uint32_t imm_11 = (ins & 0x1000) >> 1;
                    const uint32_t imm_4  = (ins & 0x800) >> 7;
                    const uint32_t imm_98 = (ins & 0x600) >> 1;
                    const uint32_t imm_10 = (ins & 0x100) << 2;
                    const uint32_t imm_6  = (ins & 0x80) >> 1;
                    const uint32_t imm_7  = (ins & 0x40) << 1;
                    const uint32_t imm_31 = (ins & 0x38) >> 2;
                    const uint32_t imm_5  = (ins & 0x4) << 3;

                    int64_t imm_final =
                        static_cast<int64_t>(imm_11 | imm_4 | imm_98 | imm_10 | imm_6 | imm_7 | imm_31 | imm_5);

                    // sign-extend the immediate if needed (is MSB 1, indicates negative)
                    if ( imm_11 != 0 ) { imm_final |= 0xFFFFFFFFFFFFF800; }

                    const int64_t  pc_i64      = static_cast<int64_t>(ins_address);
                    const uint64_t jump_target = static_cast<uint64_t>(pc_i64 + imm_final);

                    output->verbose(
                        CALL_INFO, 16, 0, "----> decode RVC JUMP pc=0x%llx + imm=%" PRId64 " = 0x%llx\n", ins_address,
                        imm_final, jump_target);

                    bundle->addInstruction(new VanadisJumpInstruction(
                        ins_address, hw_thr, options, 2, jump_target, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 0xC000:
                {
                    // BEQZ
                    uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                    const uint32_t imm_8  = (ins & 0x1000) >> 4;
                    const uint32_t imm_43 = (ins & 0xC00) >> 7;
                    const uint32_t imm_76 = (ins & 0x60) << 1;
                    const uint32_t imm_21 = (ins & 0x18) >> 2;
                    const uint32_t imm_5  = (ins & 0x4) << 3;

                    int64_t imm_final = static_cast<int64_t>(imm_8 | imm_76 | imm_5 | imm_43 | imm_21);

                    if ( imm_8 != 0 ) { imm_final |= 0xFFFFFFFFFFFFFE00; }

                    output->verbose(
                        CALL_INFO, 16, 0, "----> decode RVC BEQZ %" PRIu16 " jump to: 0x%llx + 0x%llx = 0x%llx\n",
                        rvc_rs1, ins_address, imm_final, ins_address + imm_final);

                    bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<
                                           int64_t, REG_COMPARE_EQ>(
                        ins_address, hw_thr, options, 2, rvc_rs1, 0, imm_final, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 0xE000:
                {
                    // BNEZ
                    uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                    const uint32_t imm_8  = (ins & 0x1000) >> 4;
                    const uint32_t imm_43 = (ins & 0xC00) >> 7;
                    const uint32_t imm_76 = (ins & 0x60) << 1;
                    const uint32_t imm_21 = (ins & 0x18) >> 2;
                    const uint32_t imm_5  = (ins & 0x4) << 3;

                    int64_t imm_final = static_cast<int64_t>(imm_8 | imm_76 | imm_5 | imm_43 | imm_21);

                    if ( imm_8 != 0 ) { imm_final |= 0xFFFFFFFFFFFFFE00; }

                    output->verbose(
                        CALL_INFO, 16, 0, "----> decode RVC BNEZ %" PRIu16 " jump to: 0x%llx + 0x%llx = 0x%llx\n",
                        rvc_rs1, ins_address, imm_final, ins_address + imm_final);

                    bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<
                                           int64_t, REG_COMPARE_NEQ>(
                        ins_address, hw_thr, options, 2, rvc_rs1, 0, imm_final, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                }
            } break;
            case 0x2:
            {
                const uint32_t c_func_code = ins & 0xE000;

                output->verbose(
                    CALL_INFO, 16, 0, "---> RVC function code = %" PRIu32 " / 0x%" PRIx32 "\n", c_func_code, c_func_code);

                switch ( c_func_code ) {
                case 0x0:
                {
                    // SLLI
                    uint32_t shamt_5  = (ins & 0x1000) >> 7;
                    uint32_t shamt_40 = (ins & 0x7C) >> 2;

                    uint64_t shift_by = shamt_5 | shamt_40;

                    uint16_t rvc_rs1 = (ins & 0xF80) >> 7;

                    output->verbose(
                        CALL_INFO, 16, 0, "--------> RVC SLLI %" PRIu16 " = %" PRIu16 " >> %" PRIu64 " (0x%" PRIx64 ")\n",
                        rvc_rs1, rvc_rs1, shift_by, shift_by);
                    bundle->addInstruction(
                        new VanadisShiftLeftLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                            ins_address, hw_thr, options, rvc_rs1, rvc_rs1, shift_by));
                    decode_fault = false;

                } break;
                case 0x2000:
                {
                    // FLDSP
                    uint32_t imm_86 = (ins & 0x1C) << 4;
                    uint32_t imm_43 = (ins & 0x60) >> 2;
                    uint32_t imm_5  = (ins & 0x1000) >> 7;

                    uint32_t offset = imm_5 | imm_43 | imm_86;

                    uint16_t rvc_rd = static_cast<uint16_t>((ins & 0xF80) >> 7);

                    output->verbose(
                        CALL_INFO, 16, 0, "--------> RVC FLDSP %" PRIu16 " <- memory[sp (r2) + %" PRIu32 "]\n", rvc_rd,
                        offset);
                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, 2, offset, rvc_rd, 8, false, MEM_TRANSACTION_NONE,
                        LOAD_FP_REGISTER));
                    decode_fault = false;

                } break;
                case 0x4000:
                {
                    // LWSP
                    uint32_t imm_76 = (ins & 0xC) << 4;
                    uint32_t imm_42 = (ins & 0x70) >> 2;
                    uint32_t imm_5  = (ins & 0x1000) >> 7;

                    uint32_t offset = imm_5 | imm_42 | imm_76;

                    uint16_t rvc_rd = static_cast<uint16_t>((ins & 0xF80) >> 7);

                    output->verbose(
                        CALL_INFO, 16, 0, "--------> RVC LWSP %" PRIu16 " <- memory[sp (r2) + %" PRIu32 "]\n", rvc_rd,
                        offset);

                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, 2, offset, rvc_rd, 4, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;

                } break;
                case 0x6000:
                {
                    // FLWSP
                    uint16_t rvc_rd = (ins & 0xF80) >> 7;

                    if ( 0 == rvc_rd ) {
                        // FLWSP
                        output->verbose(CALL_INFO, 16, 0, "-----> RVC FLWSP STOP\n");
                    }
                    else {
                        // LDSP
                        uint32_t offset_86 = (ins & 0x1C) << 4;
                        uint32_t offset_43 = (ins & 0x60) >> 2;
                        uint32_t offset_5  = (ins & 0x1000) >> 7;

                        int64_t offset = offset_43 | offset_5 | offset_86;

                        output->verbose(
                            CALL_INFO, 16, 0, "-----> RVC LDSP rd=%" PRIu16 " <- %" PRIu16 " + %" PRId64 "\n", rvc_rd,
                            2, offset);

                        bundle->addInstruction(new VanadisLoadInstruction(
                            ins_address, hw_thr, options, 2, offset, rvc_rd, 8, true, MEM_TRANSACTION_NONE,
                            LOAD_INT_REGISTER));
                        decode_fault = false;
                    }
                } break;
                case 0x8000:
                {
                    // Jumps
                    const uint32_t c_func_12     = ins & 0x1000;
                    const uint32_t c_func_12_rs2 = (ins & 0x7C) >> 2;
                    const uint32_t c_func_12_rs1 = (ins & 0xF80) >> 7;

                    output->verbose(
                        CALL_INFO, 16, 0, "------> RVC 12b: %" PRIu32 " / rs1=%" PRIu32 " / rs2=%" PRIu32 "\n",
                        c_func_12, c_func_12_rs1, c_func_12_rs2);

                    if ( c_func_12 == 0 ) {
                        if ( c_func_12_rs2 == 0 ) {
                            // JR unless rs1 = 0
                            output->verbose(
                                CALL_INFO, 16, 0, "--------> (generates) RVC JR %" PRIu16 "\n", c_func_12_rs1);
                            bundle->addInstruction(new VanadisJumpRegInstruction(
                                ins_address, hw_thr, options, 2, c_func_12_rs1, VANADIS_NO_DELAY_SLOT));
                            decode_fault = false;
                        }
                        else {
                            if ( c_func_12_rs1 == 0 ) {
                                // HINT (turns into a NOP?)
                            }
                            else {
                                output->verbose(
                                    CALL_INFO, 16, 0, "--------> (generates) RVC MV %" PRIu16 " <- %" PRIu16 "\n",
                                    c_func_12_rs1, c_func_12_rs2);

                                bundle->addInstruction(
                                    new VanadisAddInstruction<int64_t>(
                                        ins_address, hw_thr, options, static_cast<uint16_t>(c_func_12_rs1),
                                        static_cast<uint16_t>(c_func_12_rs2), options->getRegisterIgnoreWrites()));
                                decode_fault = false;
                            }
                        }
                    }
                    else {
                        if ( c_func_12_rs2 == 0 ) {
                            if ( c_func_12_rs1 != 0 ) {
                                // RVC JALR
                                output->verbose(
                                    CALL_INFO, 16, 0,
                                    "--------> (generates) RVC JALR link-reg: 1 / jump-to: %" PRIu16 "\n",
                                    static_cast<uint16_t>(c_func_12_rs1));
                                bundle->addInstruction(new VanadisJumpRegLinkInstruction(
                                    ins_address, hw_thr, options, 2, 1, c_func_12_rs1, 0, VANADIS_NO_DELAY_SLOT));
                                decode_fault = false;
                            }
                            else {
                                // RVC EBREAK - fault?
                                output->verbose(
                                    CALL_INFO, 16, 0,
                                    "--------> (generates) RVC EBREAK -> pipeline fault if executed\n");
                                bundle->addInstruction(new VanadisInstructionFault(
                                    ins_address, hw_thr, options, "EBREAK executed, pipeline halt/stop.\n"));
                                decode_fault = false;
                            }
                        }
                        else {
                            if ( c_func_12_rs1 == 0 ) {
                                // RVC ADD HINT - fault?
                                output->verbose(CALL_INFO, 16, 0, "--------> (generates) RVC ADD HINT\n");
                                bundle->addInstruction(
                                    new VanadisAddInstruction<int64_t>(
                                        ins_address, hw_thr, options, 0, 0, 0));
                                decode_fault = false;
                            }
                            else {
                                // ADD
                                output->verbose(
                                    CALL_INFO, 16, 0,
                                    "--------> (generates) RVC ADD %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n",
                                    c_func_12_rs1, c_func_12_rs1, c_func_12_rs2);

                                bundle->addInstruction(
                                    new VanadisAddInstruction<int64_t>(
                                        ins_address, hw_thr, options, static_cast<uint16_t>(c_func_12_rs1),
                                        static_cast<uint16_t>(c_func_12_rs1), static_cast<uint16_t>(c_func_12_rs2)));
                                decode_fault = false;
                            }
                        }
                    }
                } break;
                case 0xA000:
                {
                    // FSDSP
                    uint16_t rvc_src = (ins & 0x7C) >> 2;

                    uint32_t offset_53 = (ins & 0x1C00) >> 7;
                    uint32_t offset_86 = (ins & 0x380) >> 1;

                    uint32_t offset = offset_86 | offset_53;

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC FSDSP %" PRIu16 " -> memory[ sp + %" PRIu32 " ]\n", rvc_src,
                        offset);
                    bundle->addInstruction(new VanadisStoreInstruction(
                        ins_address, hw_thr, options, 2, offset, rvc_src, 8, MEM_TRANSACTION_NONE, STORE_FP_REGISTER));
                    decode_fault = false;
                } break;
                case 0xC000:
                {
                    // SWSP
                    uint16_t rvc_src = (ins & 0x7C) >> 2;

                    uint32_t offset_52 = (ins & 0x1E00) >> 7;
                    uint32_t offset_76 = (ins & 0x180) >> 1;

                    uint32_t offset = offset_76 | offset_52;

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC SWSP %" PRIu16 " -> %" PRIu16 " + %" PRIu64 "\n", rvc_src, (uint16_t)2,
                        offset);

                    bundle->addInstruction(new VanadisStoreInstruction(
                        ins_address, hw_thr, options, 2, offset, rvc_src, 4, MEM_TRANSACTION_NONE, STORE_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 0xE000:
                {
                    // SDSP
                    uint16_t rvc_src = (ins & 0x7C) >> 2;
                    ;

                    uint32_t offset_53 = (ins & 0x1C00) >> 7;
                    uint32_t offset_86 = (ins & 0x380) >> 1;

                    uint32_t offset = offset_86 | offset_53;

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC SDSP %" PRIu16 " -> %" PRIu16 " + %" PRIu64 "\n", rvc_src, (uint16_t)2,
                        offset);

                    bundle->addInstruction(new VanadisStoreInstruction(
                        ins_address, hw_thr, options, 2, offset, rvc_src, 8, MEM_TRANSACTION_NONE, STORE_INT_REGISTER));
                    decode_fault = false;
                } break;
                }
            } break;
            }

            output->verbose(
                CALL_INFO, 16, 0, "[decode] -> 16bit RVC format / ins-op-code-family: %" PRIu32 " / 0x%x\n", op_code,
                op_code);
            //            if ( decode_fault ) { output->fatal(CALL_INFO, -1, "STOP\n"); }
        }


        if ( decode_fault ) {
            if ( fatal_decode_fault ) {
                output->fatal(
                    CALL_INFO, -1,
                    "[decode] -> decode fault detected at 0x%llx / thr: %" PRIu32 ", set to fatal on detect\n",
                    ins_address, hw_thr);
            }
            bundle->addInstruction(new VanadisInstructionDecodeFault(ins_address, hw_thr, options));
        }
    }

    uint16_t expand_rvc_int_register(const uint16_t reg_in) const { return reg_in + 8; }

    uint16_t extract_rs2_rvc(const uint32_t ins) const { return static_cast<uint16_t>((ins & 0x1C) >> 2); }

    uint16_t extract_rs1_rvc(const uint32_t ins) const { return static_cast<uint16_t>((ins & 0x380) >> 7); }

    uint64_t extract_uimm_rvc_t1(const uint32_t ins) const
    {
        const uint64_t uimm_76 = ((ins & 0x60) << 1);
        const uint64_t uimm_53 = ((ins & 0x1C00) >> 7);

        return (uimm_76 | uimm_53);
    }

    uint64_t extract_uimm_rcv_t2(const uint32_t ins) const
    {
        const uint64_t uimm_6  = ((ins & 0x20) << 1);
        const uint64_t uimm_2  = ((ins & 0x40) >> 4);
        const uint64_t uimm_53 = ((ins & 0x1C00) >> 7);

        return (uimm_6 | uimm_53 | uimm_2);
    }

    uint64_t extract_uimm_rcv_t3(const uint32_t ins) const
    {
        const uint64_t uimm_76 = ((ins & 0x60) << 1);
        const uint64_t uimm_54 = ((ins & 0x1800) >> 6);
        const uint64_t uimm_8  = ((ins & 0x400) >> 2);

        return (uimm_76 | uimm_54 | uimm_8);
    }

    uint64_t extract_nzuimm_rcv(const uint32_t ins) const
    {
        const uint64_t imm_3  = ((ins & 0x20) >> 2);
        const uint64_t imm_2  = ((ins & 0x40) >> 4);
        const uint64_t imm_96 = ((ins & 0x780) >> 1);
        const uint64_t imm_54 = ((ins & 0x1800) >> 6);

        return (imm_96 | imm_54 | imm_3 | imm_2);
    }

    uint16_t extract_rd(const uint32_t ins) const { return static_cast<uint16_t>((ins & VANADIS_RISCV_RD_MASK) >> 7); }

    uint16_t extract_rs1(const uint32_t ins) const
    {
        return static_cast<uint16_t>((ins & VANADIS_RISCV_RS1_MASK) >> 15);
    }

    uint16_t extract_rs2(const uint32_t ins) const
    {
        return static_cast<uint16_t>((ins & VANADIS_RISCV_RS2_MASK) >> 20);
    }

    uint32_t extract_func3(const uint32_t ins) const { return ((ins & VANADIS_RISCV_FUNC3_MASK) >> 12); }

    uint32_t extract_func7(const uint32_t ins) const { return ((ins & VANADIS_RISCV_FUNC7_MASK) >> 25); }

    uint32_t extract_opcode(const uint32_t ins) const { return (ins & VANADIS_RISCV_OPCODE_MASK); }

    int32_t extract_imm12(const uint32_t ins) const
    {
        const uint32_t imm12 = (ins & VANADIS_RISCV_IMM12_MASK) >> 20;
        return sign_extend12(imm12);
    }

    int64_t sign_extend12(int64_t value) const
    {
        return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? value : (value | 0xFFFFFFFFFFFFF000LL);
    }

    int64_t sign_extend12(uint64_t value) const
    {
        // If sign at bit-12 is 1, then set all values 31:20 to 1
        return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? static_cast<int64_t>(value)
                                                        : (static_cast<int64_t>(value) | 0xFFFFFFFFFFFFF000LL);
    }

    int32_t sign_extend12(int32_t value) const
    {
        // If sign at bit-12 is 1, then set all values 31:20 to 1
        return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? value : (value | 0xFFFFF000);
    }

    int32_t sign_extend12(uint32_t value) const
    {
        // If sign at bit-12 is 1, then set all values 31:20 to 1
        return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? static_cast<int32_t>(value)
                                                        : (static_cast<int32_t>(value) | 0xFFFFF000);
    }

    // Extract components for an R-type instruction
    void processR(
        const uint32_t ins, uint32_t& opcode, uint16_t& rd, uint16_t& rs1, uint16_t& rs2, uint32_t& func_code_1,
        uint32_t& func_code_2) const
    {
        opcode      = extract_opcode(ins);
        rd          = extract_rd(ins);
        rs1         = extract_rs1(ins);
        rs2         = extract_rs2(ins);
        func_code_1 = extract_func3(ins);
        func_code_2 = extract_func7(ins);
    }

    // Extract components for an I-type instruction
    template <typename T>
    void processI(const uint32_t ins, uint32_t& opcode, uint16_t& rd, uint16_t& rs1, uint32_t& func_code, T& imm) const
    {
        opcode    = extract_opcode(ins);
        rd        = extract_rd(ins);
        rs1       = extract_rs1(ins);
        func_code = extract_func3(ins);

        // This also performs sign extension which is required in the RISC-V ISA
        int32_t imm_tmp = extract_imm12(ins);
        imm             = static_cast<T>(imm_tmp);
    }

    template <typename T>
    void processS(const uint32_t ins, uint32_t& opcode, uint16_t& rs1, uint16_t& rs2, uint32_t& func_code, T& imm) const
    {
        opcode    = extract_opcode(ins);
        rs1       = extract_rs1(ins);
        rs2       = extract_rs2(ins);
        func_code = extract_func3(ins);

        const int32_t ins_i32 = static_cast<int32_t>(ins);
        imm                   = static_cast<T>(
            sign_extend12(((ins_i32 & VANADIS_RISCV_RD_MASK) >> 7) | ((ins_i32 & VANADIS_RISCV_FUNC7_MASK) >> 20)));
    }

    template <typename T>
    void processU(const uint32_t ins, uint32_t& opcode, uint16_t& rd, T& imm) const
    {
        opcode = extract_opcode(ins);
        rd     = extract_rd(ins);

        const int32_t ins_i32 = static_cast<int32_t>(ins);
        imm                   = static_cast<T>(ins_i32 & 0xFFFFF000);
    }

    template <typename T>
    void processJ(const uint32_t ins, uint32_t& opcode, uint16_t& rd, T& imm) const
    {
        opcode = extract_opcode(ins);
        rd     = extract_rd(ins);

        const uint32_t ins_i32  = static_cast<uint32_t>(ins);
        const uint32_t imm20    = (ins_i32 & 0x80000000) >> 11;
        const uint32_t imm19_12 = (ins_i32 & 0xFF000);
        const uint32_t imm_10_1 = (ins_i32 & 0x7FE00000) >> 20;
        const uint32_t imm11    = (ins_i32 & 0x100000UL) >> 9;

        int32_t imm_tmp = (imm20 | imm19_12 | imm11 | imm_10_1);
        imm_tmp         = (0 == imm20) ? imm_tmp : imm_tmp | 0xFFF00000;

        imm = static_cast<T>(imm_tmp);
    }

    template <typename T>
    void processB(const uint32_t ins, uint32_t& opcode, uint16_t& rs1, uint16_t& rs2, uint32_t& func_code, T& imm) const
    {
        opcode    = extract_opcode(ins);
        rs1       = extract_rs1(ins);
        rs2       = extract_rs2(ins);
        func_code = extract_func3(ins);

        const int32_t ins_i32 = static_cast<int32_t>(ins);

        const int32_t imm_12  = (ins_i32 & 0x80000000) >> 19;
        const int32_t imm_11  = (ins_i32 & 0x80) << 4;
        const int32_t imm_105 = (ins_i32 & 0x7E000000) >> 20;
        const int32_t imm_41  = (ins_i32 & 0xF00) >> 7;

        int32_t imm_tmp = imm_12 | imm_11 | imm_105 | imm_41;

        if ( imm_12 != 0 ) { imm_tmp |= 0xFFFFF000; }

        imm = static_cast<T>(imm_tmp);
    }
};
} // namespace Vanadis
} // namespace SST

#endif
