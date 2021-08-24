
#ifndef _H_VANADIS_RISCV64_DECODER
#define _H_VANADIS_RISCV64_DECODER

#include <cstdint>
#include <cstring>

#include "decoder/vdecoder.h"
#include "inst/vinstall.h"

#define VANADIS_RISCV_OPCODE_MASK 0x7F
#define VANADIS_RISCV_RD_MASK 0xF80
#define VANADIS_RISCV_RS1_MASK 0xF8000
#define VANADIS_RISCV_RS2_MASK 0x1F00000
#define VANADIS_RISCV_FUNC3_MASK 0x7000
#define VANADIS_RISCV_FUNC7_MASK 0xFE000000
#define VANADIS_RISCV_IMM12_MASK 0xFFF00000
#define VANADIS_RISCV_IMM7_MASK 0xFE000000
#define VANADIS_RISCV_IMM5_MASK 0xF80
#define VANADIS_RISCV_IMM20_MASK 0xFFFFF000

#define VANADIS_RISCV_SIGN12_MASK 0x800
#define VANADIS_RISCV_SIGN12_UPPER_1_32 0xFFFFF000
#define VANADIS_RISCV_SIGN12_UPPER_1_64 0xFFFFFFFFFFFFF000LL

namespace SST {
namespace Vanadis {

class VanadisRISCV64Decoder : public VanadisDecoder {
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
      {"stack_start_address",
       "Sets the start of the stack and dynamic program segments"})

  VanadisRISCV64Decoder(ComponentId_t id, Params &params)
      : VanadisDecoder(id, params) {
    options = new VanadisDecoderOptions(static_cast<uint16_t>(0), 32, 32, 2,
                                        VANADIS_REGISTER_MODE_FP64);
    max_decodes_per_cycle =
        params.find<uint16_t>("decode_max_ins_per_cycle", 2);

    // MIPS default is 0x7fffffff according to SYS-V manual
    start_stack_address =
        params.find<uint64_t>("stack_start_address", 0x7ffffff0);

    // See if we get an entry point the sub-component says we have to use
    // if not, we will fall back to ELF reading at the core level to work this
    // out
    setInstructionPointer(params.find<uint64_t>("entry_point", 0));
  }

  ~VanadisRISCV64Decoder() {}

  const char *getISAName() const override { return "RISCV64"; }
  uint16_t countISAIntReg() const override {
    return options->countISAIntRegisters();
  }
  uint16_t countISAFPReg() const override {
    return options->countISAFPRegisters();
  }
  const VanadisDecoderOptions *getDecoderOptions() const override {
    return options;
  }
  VanadisFPRegisterMode getFPRegisterMode() const override {
    return VANADIS_REGISTER_MODE_FP64;
  }

  void configureApplicationLaunch(SST::Output *output, VanadisISATable *isa_tbl,
                                  VanadisRegisterFile *regFile,
                                  VanadisLoadStoreQueue *lsq,
                                  VanadisELFInfo *elf_info,
                                  SST::Params &params) override {}

  void tick(SST::Output *output, uint64_t cycle) override {
    output->verbose(CALL_INFO, 16, 0, "-> Decode step for thr: %" PRIu32 "\n",
                    hw_thr);
    output->verbose(CALL_INFO, 16, 0,
                    "---> Max decodes per cycle: %" PRIu16 "\n",
                    max_decodes_per_cycle);

    for (uint16_t i = 0; i < max_decodes_per_cycle; ++i) {
      if (!thread_rob->full()) {
        if (ins_loader->hasBundleAt(ip)) {
          // We have the instruction in our micro-op cache
          output->verbose(
              CALL_INFO, 16, 0,
              "---> Found uop bundle for ip=0x%llx, loading from cache...\n",
              ip);

          VanadisInstructionBundle *bundle = ins_loader->getBundleAt(ip);
          output->verbose(CALL_INFO, 16, 0,
                          "----> Bundle contains %" PRIu32 " entries.\n",
                          bundle->getInstructionCount());

          // Do we have enough space in the ROB to push the micro-op bundle into
          // the queue?
          if (bundle->getInstructionCount() <
              (thread_rob->capacity() - thread_rob->size())) {
            bool bundle_has_branch = false;

            for (uint32_t i = 0; i < bundle->getInstructionCount(); ++i) {
              VanadisInstruction *next_ins = bundle->getInstructionByIndex(i);

              if (next_ins->getInstFuncType() == INST_BRANCH) {
                VanadisSpeculatedInstruction *next_spec_ins =
                    dynamic_cast<VanadisSpeculatedInstruction *>(next_ins);

                if (branch_predictor->contains(ip)) {
                  // We have an address predicton from the branching unit
                  const uint64_t predicted_address =
                      branch_predictor->predictAddress(ip);
                  next_spec_ins->setSpeculatedAddress(predicted_address);

                  output->verbose(CALL_INFO, 16, 0,
                                  "----> contains a branch: 0x%llx / predicted "
                                  "(found in predictor): 0x%llx\n",
                                  ip, predicted_address);

                  ip = predicted_address;
                  bundle_has_branch = true;
                } else {
                  // We don't have an address prediction
                  // so just speculate that we are going to drop through to the
                  // next instruction as we aren't sure where this will go yet

                  output->verbose(CALL_INFO, 16, 0,
                                  "----> contains a branch: 0x%llx / predicted "
                                  "(not-found in predictor): 0x%llx\n",
                                  ip, ip + 4);

                  ip += 4;
                  next_spec_ins->setSpeculatedAddress(ip);
                  bundle_has_branch = true;
                }
              }

              thread_rob->push(next_ins->clone());
            }

            // Move to the next address, if we had a branch we should have
            // already found a predicted target addeess to decode
            ip = bundle_has_branch ? ip : ip + 4;
          } else {
            output->verbose(
                CALL_INFO, 16, 0,
                "----> Not enough space in the ROB, will stall this cycle.\n");
          }
        } else if (ins_loader->hasPredecodeAt(ip)) {
          // We have a loaded instruction cache line but have not decoded it yet
          output->verbose(CALL_INFO, 16, 0,
                          "---> uop not found, but is located in the predecode "
                          "i0-icache (ip=0x%llx)\n",
                          ip);
          VanadisInstructionBundle *decoded_bundle =
              new VanadisInstructionBundle(ip);

          uint32_t temp_ins = 0;
          if (ins_loader->getPredecodeBytes(output, ip, (uint8_t *)&temp_ins,
                                            sizeof(temp_ins))) {
            output->verbose(CALL_INFO, 16, 0,
                            "---> performing a decode for ip=0x%llx\n", ip);
            decode(output, ip, temp_ins, decoded_bundle);

            output->verbose(CALL_INFO, 16, 0,
                            "---> bundle generates %" PRIu32 " micro-ops\n",
                            (uint32_t)decoded_bundle->getInstructionCount());

            ins_loader->cacheDecodedBundle(decoded_bundle);

            // Exit this cycle because results saved to cache are available next
            // cycle
            break;
          } else {
            output->fatal(CALL_INFO, -1,
                          "Error - predecoded bytes for 0x%llu found, but "
                          "retrieval of bytes failed.\n",
                          ip);
          }
        } else {
          // Not in micro or predecode cache, so we have to regenrata a request
          // and stop further processing
          output->verbose(
              CALL_INFO, 16, 0,
              "---> microop bundle and pre-decoded bytes are not found for "
              "0x%llx, requested read for cache line (line=%" PRIu64 ")\n",
              ip, ins_loader->getCacheLineWidth());
          ins_loader->requestLoadAt(output, ip, 4);
          break;
        }
      } else {
        output->verbose(CALL_INFO, 16, 0,
                        "---> Decode pending queue (ROB) is full, no more "
                        "decoded permitted this cycle.\n");
      }
    }

    output->verbose(CALL_INFO, 16, 0, "---> cycle is completed, ip=0x%llx\n",
                    ip);
  }

protected:
  const VanadisDecoderOptions *options;
  uint64_t start_stack_address;
  uint16_t icache_max_bytes_per_cycle;
  uint16_t max_decodes_per_cycle;
  uint16_t decode_buffer_max_entries;

  void decode(SST::Output *output, const uint64_t ins_address,
              const uint32_t ins, VanadisInstructionBundle *bundle) {

    output->verbose(CALL_INFO, 16, 0, "[decode -> addr: 0x%llx / ins: 0x%08x\n",
                    ins_address, ins);

    const uint32_t op_code = extract_opcode(ins);

    uint16_t rd = 0;
    uint16_t rs1 = 0;
    uint16_t rs2 = 0;

    uint32_t func_code = 0;
    uint32_t func_code3 = 0;
    uint32_t func_code7 = 0;
    uint64_t uimm64 = 0;
    int64_t simm64 = 0;

    bool decode_fault = true;

    switch (op_code) {
    case 0x3: {
      // Load data
      processI<int64_t>(ins, op_code, rd, rs1, func_code, simm64);

      switch (func_code) {
      case 0: {
        // LB
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_address, hw_thr, options, rs1, simm64, rd, 1, true,
            MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        decode_fault = false;
      } break;
      case 1: {
        // LH
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_address, hw_thr, options, rs1, simm64, rd, 2, true,
            MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        decode_fault = false;
      } break;
      case 2: {
        // LW
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_address, hw_thr, options, rs1, simm64, rd, 4, true,
            MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        decode_fault = false;
      } break;
      case 3: {
        // LD
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_address, hw_thr, options, rs1, simm64, rd, 8, true,
            MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        decode_fault = false;
      } break;
      case 4: {
        // LBU
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_address, hw_thr, options, rs1, simm64, rd, 1, false,
            MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        decode_fault = false;
      } break;
      case 5: {
        // LHU
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_address, hw_thr, options, rs1, simm64, rd, 2, false,
            MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        decode_fault = false;
      } break;
      case 6: {
        // LWU
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_address, hw_thr, options, rs1, simm64, rd, 4, false,
            MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        decode_fault = false;
      } break;
      }
    } break;
    case 0x7: {
      // Floating point load
    } break;
    case 0x23: {
      // Store data
      processS<int64_t>(ins, op_code, rs1, rs2, func_code3, simm64);

      if (func_code < 4) {
        // shift to get the power of 2 number of bytes to store
        const uint32_t store_bytes = 1 << func_code;
        bundle->addInstruction(new VanadisStoreInstruction(
            ins_address, hw_thr, options, rs1, simm64, rs2, store_bytes,
            MEM_TRANSACTION_NONE, STORE_INT_REGISTER));
        decode_fault = false;
      }
    } break;
    case 0x13: {
      // Immediate arithmetic
      func_code = extract_func3(ins);

      switch (func_code) {
      case 0: {
        // ADDI
        processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);

        bundle->addInstruction(new VanadisAddImmInstruction<
                               VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
            ins_address, hw_thr, options, rd, rs1, simm64));
        decode_fault = false;
      } break;
      case 1: {
        // SLLI
        processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

        switch (func_code7) {
        case 0: {
          bundle->addInstruction(new VanadisShiftLeftLogicalImmInstruction<
                                 VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
              ins_address, hw_thr, options, rs1, rd,
              static_cast<uint64_t>(rs2)));
          decode_fault = false;
        } break;
        };
      } break;
      case 2: {
        // SLTI
        processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);
        bundle->addInstruction(
            new VanadisSetRegCompareImmInstruction<
                REG_COMPARE_LT, VanadisRegisterFormat::VANADIS_FORMAT_INT32,
                true>(ins_address, hw_thr, options, rd, rs1, simm64));
        decode_fault = false;
      } break;
      case 3: {
        // SLTIU
        processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);
        bundle->addInstruction(
            new VanadisSetRegCompareImmInstruction<
                REG_COMPARE_LT, VanadisRegisterFormat::VANADIS_FORMAT_INT32,
                false>(ins_address, hw_thr, options, rd, rs1, simm64));
        decode_fault = false;
      } break;
      case 4: {
        // XORI
        processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);
        bundle->addInstruction(new VanadisXorImmInstruction(
            ins_address, hw_thr, options, rd, rs1, simm64));
        decode_fault = false;
      } break;
      case 5: {
        // CHECK SRLI / SRAI
        processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

        switch (func_code7) {
        case 0: {
          bundle->addInstruction(new VanadisShiftRightLogicalImmInstruction<
                                 VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
              ins_address, hw_thr, options, rs1, rd,
              static_cast<uint64_t>(rs2)));
          decode_fault = false;
        } break;
        case 0x20: {
          bundle->addInstruction(new VanadisShiftRightArithmeticImmInstruction<
                                 VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
              ins_address, hw_thr, options, rs1, rd,
              static_cast<uint64_t>(rs2)));
          decode_fault = false;
        } break;
        };
      } break;
      case 6: {
        // ORI
        processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);
        bundle->addInstruction(new VanadisOrImmInstruction(
            ins_address, hw_thr, options, rd, rs1, simm64));
        decode_fault = false;
      } break;
      case 7: {
        // ANDI
        processI<int64_t>(ins, op_code, rd, rs1, func_code3, simm64);
        bundle->addInstruction(new VanadisAndImmInstruction(
            ins_address, hw_thr, options, rd, rs1, simm64));
        decode_fault = false;
      } break;
      };
    } break;
    case 0x33: {
      // 32b integer arithmetic/logical
      processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

      switch (func_code3) {
      case 0: {
        switch (func_code7) {
        case 1: {
          // MUL
          // TODO - check register ordering
          bundle->addInstruction(new VanadisMultiplyInstruction<
                                 VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
              ins_address, hw_thr, options, rs1, rs2, rd));
          decode_fault = false;
        } break;
        };
      } break;
      case 1: {
        switch (func_code7) {
        case 1: {
          // MULH
          // NOT SURE WHAT THIS IS?
        } break;
        };
      } break;
      case 2: {
        switch (func_code7) {
        case 1: {
          // MULHSU
          // NOT SURE WHAT THIS IS?
        } break;
        };
      } break;
      case 3: {
        switch (func_code7) {
        case 1: {
          // MULHSU
          // NOT SURE WHAT THIS IS?
        } break;
        };
      } break;
      case 4: {
        switch (func_code7) {
        case 1: {
          // DIV
          // CHECK REGISTER ORDERING
          bundle->addInstruction(new VanadisDivideInstruction<
                                 VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
              ins_address, hw_thr, options, rs1, rs2, rd));
          decode_fault = false;
        } break;
        };
      } break;
      case 5: {
        switch (func_code7) {
        case 1: {
          // DIVU
          // NOT SURE WHAT THIS IS?
        } break;
        };
      } break;
      case 6: {
        // TODO - REM INSTRUCTION
      } break;
      case 7: {
        // TODO - REMU INSTRUCTION
      } break;
      };
    } break;
    case 0x37: {
      // LUI
      processU<int64_t>(ins, rd, simm64);

      bundle->addInstruction(new VanadisSetRegisterInstruction<
                             VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
          ins_address, hw_thr, options, rd, simm64));
      decode_fault = false;
    } break;
    case 0x17: {
      // AUIPC
      processU<int64_t>(ins, rd, simm64);

      bundle->addInstruction(new VanadisPCAddImmInstruction<
                             VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
          ins_address, hw_thr, options, rd, simm64));
      decode_fault = false;
    } break;
    case 0x6F: {
      // JAL
      processJ<int64_t>(ins, rd, simm64);
      // Immediate specifies jump in multiples of 2 byts per RISCV spec
      const int64_t jump_to = static_cast<int64_t>(ins_address) + (simm64 << 1);

      bundle->addInstruction(new VanadisJumpLinkInstruction(
          ins_address, hw_thr, rd, jump_to, VANADIS_NO_DELAY_SLOT));
      decode_fault = false;
    } break;
    case 0x67: {
      // JALR
      processI<int64_t>(ins, rd, rs1, func_code3, simm64);

      switch (func_code3) {
      case 0: {
        // TODO - may need to zero bit 1 with an AND microop?
        bundle->addInstruction(new VanadisJumpRegLinkInstruction(
            ins_address, hw_thr, options, rd, rs1, VANADIS_NO_DELAY_SLOT));
        decode_fault = false;
      } break;
      };
    } break;
    case 0x63: {
      // Branch
      processB<uint64_t>(ins, rs1, rs2, func_code, imm64);

      switch (func_code) {
      case 0: {
        // BEQ
        bundle->addInstruction(
            new VanadisBranchRegCompareInstruction<
                VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_EQ>(
                ins_address, hw_thr, options, rs1, rs2, imm64,
                VANADIS_NO_DELAY_SLOT));
        decode_fault = false;
      } break;
      case 1: {
        // BNE
        bundle->addInstruction(
            new VanadisBranchRegCompareInstruction<
                VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_NEQ>(
                ins_address, hw_thr, options, rs1, rs2, imm64,
                VANADIS_NO_DELAY_SLOT));
        decode_fault = false;
      } break;
      case 4: {
        // BLT
        bundle->addInstruction(
            new VanadisBranchRegCompareInstruction<
                VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_LT>(
                ins_address, hw_thr, options, rs1, rs2, imm64,
                VANADIS_NO_DELAY_SLOT));
        decode_fault = false;
      } break;
      case 5: {
        // BGE
        bundle->addInstruction(
            new VanadisBranchRegCompareInstruction<
                VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_GTE>(
                ins_address, hw_thr, options, rs1, rs2, imm64,
                VANADIS_NO_DELAY_SLOT));
        decode_fault = false;
      } break;
      case 6: {
        // BLTU
        bundle->addInstruction(
            new VanadisBranchRegCompareInstruction<
                VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_LT>(
                ins_address, hw_thr, options, rs1, rs2, imm64,
                VANADIS_NO_DELAY_SLOT, false));
        decode_fault = false;
      } break;
      case 7: {
        // BGEU
        bundle->addInstruction(
            new VanadisBranchRegCompareInstruction<
                VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_GTE>(
                ins_address, hw_thr, options, rs1, rs2, imm64,
                VANADIS_NO_DELAY_SLOT, false));
        decode_fault = false;
      } break;
      };
    } break;
    case 0x73: {
      // Syscall/ECALL and EBREAK
      // Control registers
    } break;
    case 0x3B: {
      // 64b integer arithmetic-W
      processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

      switch (func_code3) {
      case 0: {
        switch (func_code7) {
        case 0x0: {
          // ADDW
          // TODO - check register ordering
          bundle->addInstruction(
              new VanadisAddInstruction<
                  VanadisRegisterFormat::VANADIS_FORMAT_INT64, true>(
                  ins_address, hw_thr, options, rs1, rs2, rd));
          decode_fault = false;
        } break;
        case 0x1: {
          // MULW
          // TODO - check register ordering
          bundle->addInstruction(new VanadisMultiplyInstruction<
                                 VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
              ins_address, hw_thr, options, rs1, rs2, rd));
        } break;
        case 0x20: {
          // SUBW
          // TODO - check register ordering
          bundle->addInstruction(
              new VanadisSubInstructionVanadisRegisterFormat::
                  VANADIS_FORMAT_INT64 >
              (ins_address, hw_thr, options, rs1, rs2, rd, true));
          decode_fault = false;
        } break;
        };
      } break;
      case 1: {
        switch (func_code7) {
        case 0x0: {
          // SLLW
          // TODO - check register ordering
          bundle->addInstruction(new VanadisShiftLeftLogicalInstruction<
                                 VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
              ins_address, hw_thr, options, rs1, rs2, rd));
          decode_fault = false;
        } break;
        };
      } break;
      case 4: {
        switch (func_code7) {
        case 0x1: {
          // DIVW
          // TODO - check register ordering
          bundle->addInstruction(new VanadisDivideInstruction<
                                 VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
              ins_address, hw_thr, options, rs1, rs2, rd));
          decode_fault = false;
        } break;
        };
      } break;
      case 5: {
        switch (func_code7) {
        case 0x0: {
          // SRLW
          // TODO - check register ordering
          bundle->addInstruction(new VanadisShiftRightLogicalInstruction<
                                 VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
              ins_address, hw_thr, options, rs1, rs2, rd));
          decode_fault = false;
        } break;
        case 0x1: {
          // DIVUW
          // DIVIDE UNSIGNED
        } break;
        case 0x20: {
          // SRAW
          // TODO - check register ordering
          bundle->addInstruction(new VanadisShiftRightArithmeticInstruction<
                                 VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
              ins_address, hw_thr, options, rs1, rs2, rd));
          decode_fault = false;
        } break;
        };
      } break;
      case 6: {
        switch (func_code7) {
        case 0x1: {
          // REMW
          // TODO - Implement
        } break;
        };
      } break;
      case 7: {
        switch (func_code7) {
        case 0x1: {
          // REMUW
          // TODO - Implement
        } break;
        }
      }; break;
      };
    } break;
    case 0xF: {
      // Fences (FENCE.I Zifencei)
    }
    case 0x2F: {
      // Atomic operations (A extension)
    } break;
    case 0x27: {
      // Floating point store
    } break;
    case 0x53: {
      // floating point arithmetic
    } break;
    case 0x43: {
      // FMADD
    } break;
    case 0x47: {
      // FMSUB
    } break;
    case 0x4B: {
      // FNMSUB
    } break;
    case 0x4F: {
      // FNMADD
    } break;
    default: {
      // op_code did not match an expected value
    } break;
    }
  }

  uint16_t extract_rd(const uint32_t ins) const {
    return static_cast<uint16_t>((ins & VANADIS_RISCV_RD_MASK) >> 6);
  }

  uint16_t extract_rs1(const uint32_t ins) const {
    return static_cast<uint16_t>((ins & VANADIS_RISCV_RS1_MASK) >> 14);
  }

  uint16_t extract_rs2(const uint32_t ins) const {
    return static_cast<uint16_t>((ins & VANADIS_RISCV_RS1_MASK) >> 19);
  }

  uint32_t extract_func3(const uint32_t ins) const {
    return ((ins & VANADIS_RISCV_FUNC3_MASK) >> 11);
  }

  uint32_t extract_func7(const uint32_t ins) const {
    return ((ins & VANADIS_RISCV_FUNC7_MASK) >> 24);
  }

  uint32_t extract_opcode(const uint32_t ins) const {
    return (ins * VANADIS_RISCV_OPCODE_MASK);
  }

  int32_t extract_imm12(const uint32_t ins) const {
    const uint32_t imm12 = (ins & VANADIS_RISCV_IMM12_MASK) >> 20;
    return sign_extend12(imm12);
  }

  int64_t sign_extend12(int64_t value) const {
    return (value & VANADIS_RISCV_SIGN12_MASK) == 0
               ? value
               : (value | 0xFFFFFFFFFFFFF000LL);
  }

  int64_t sign_extend12(uint64_t value) const {
    // If sign at bit-12 is 1, then set all values 31:20 to 1
                return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? static_cast<int64_t>(value) :
			(static_cast<int64_t>)(value) | 0xFFFFFFFFFFFFF000LL);
  }

  int32_t sign_extend12(int32_t value) const {
    // If sign at bit-12 is 1, then set all values 31:20 to 1
    return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? value
                                                    : (value | 0xFFFFF000);
  }

  int32_t sign_extend12(uint32_t value) const {
    // If sign at bit-12 is 1, then set all values 31:20 to 1
                return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? static_cast<int32_t>(value) :
			(static_cast<int32_t>)(value) | 0xFFFFF000);
  }

  // Extract components for an R-type instruction
  void processR(const uint32_t ins, uint32_t &opcode, uint16_t &rd,
                uint16_t &rs1, uint16_t &rs2, uint32_t &func_code_1,
                uint32_t &func_code_2) const {
    opcode = extract_opcode(ins);
    rd = extract_rd(ins);
    rs1 = extract_rs1(ins);
    rs2 = extract_rs2(ins);
    func_code_1 = extract_func3(ins);
    func_code_2 = extract_func7(ins);
  }

  // Extract components for an I-type instruction
  template <typename T>
  void processI(const uint32_t ins, uint32_t &opcode, uint16_t &rd,
                uint16_t &rs1, uint32_t &func_code, T &imm) const {
    opcode = extract_opcode(ins);
    rd = extract_rd(ins);
    rs1 = extract_rs1(ins);
    func_code = extract_func3(ins);

    // This also performs sign extension which is required in the RISC-V ISA
    int32_t imm_tmp = extract_imm12(ins);
    imm = static_cast<T>(imm_tmp);
  }

  template <typename T>
  void processS(const uint32_t ins, uint16_t &rs1, uint16_t &rs2,
                uint32_t &func_code, T &imm) const {
    opcode = extract_opcode(ins);
    rs1 = extract_rs1(ins);
    rs2 = extract_rs2(ins);
    func_code = extract_func3(ins);

    const int32_t ins_i32 = static_cast<int32_t>(ins);
    imm = static_cast<T>(
        sign_extend12(((ins_i32 & VANADIS_RISCV_RD_MASK) >> 6) |
                      ((ins_i32 & VANADIS_RISCV_FUNC7_MASK) >> 24)));
  }

  template <typename T>
  void processU(const uint32_t ins, uint16_t &rd, T &imm) const {
    opcode = extract_opcode(ins);
    rd = extract_rd(ins);

    const int32_t ins_i32 = static_cast<int32_t>(ins);
    imm = static_cast<T>(ins_i32 & 0xFFFFF000);
  }

  template <typename T>
  void processJ(const uint32_t ins, uint16_t &rd, T &imm) const {
    opcode = extract_opcode(ins);
    rd = extract_rd(ins);

    const int32_t ins_i32 = static_cast<int32_t>(ins);
    const int32_t imm20 = (ins_i32 & 0x80000000) >> 11;
    const int32_t imm19_12 = (ins_i32 & 0xFF000);
    const int32_t imm_10_1 = (ins_i32 & 0x3FF00000) >> 19;
    const int32_t imm11 = (ins_i32 & 0x80000) >> 8;

    int32_t imm_tmp = (imm20 | imm19_12 | imm11 | imm_10_1);
    imm_tmp = (0 == imm20) ? imm_tmp : imm_tmp | 0xFFF00000;

    imm = static_cast<T>(imm_tmp);
  }

  template <typename T>
  void processB(const uint32_t ins, uint16_t &rs1, uint16_t &rs2,
                uint32_t &func_code, T &imm) const {
    opcode = extract_opcode(ins);
    rs1 = extract_rs1(ins);
    rs2 = extract_rs2(ins);
    func_code = extract_func3(ins);

    const int32_t ins_i32 = static_cast<int32_t>(ins);
    const int32_t imm20 = (ins_i32 & 0x80000000) >> 31;
    const int32_t imm10_5 = (ins_i32 & 0x7E000000) >> 20;
    const int32_t imm11 = (ins_i32 & 0x100000) >> 19;
    const int32_t imm19_12 = (ins_i32 & 0xFF000) >> 11;
    const int32_t imm4_1 = (ins_i32 & 0x1F00) >> 7;

    int32_t imm_tmp = (imm20 | imm19_12 | imm11 | imm10_5 | imm_4_1);
    imm_tmp = (0 == imm20) ? imm_tmp : imm_tmp | 0xFFF00000;

    imm = static_cast<T>(imm_tmp);
  }
};
} // namespace Vanadis
} // namespace SST

#endif
