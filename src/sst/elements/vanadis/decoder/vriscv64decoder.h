
#ifndef _H_VANADIS_RISCV64_DECODER
#define _H_VANADIS_RISCV64_DECODER

#include "decoder/vdecoder.h"
#include "inst/vinstall.h"

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
      {"stack_start_address",
       "Sets the start of the stack and dynamic program segments"})

    VanadisRISCV64Decoder(ComponentId_t id, Params& params) : VanadisDecoder(id, params)
    {
        options = new VanadisDecoderOptions(static_cast<uint16_t>(0), 32, 32, 2, VANADIS_REGISTER_MODE_FP64);
        max_decodes_per_cycle = params.find<uint16_t>("decode_max_ins_per_cycle", 2);

        // MIPS default is 0x7fffffff according to SYS-V manual
        start_stack_address = params.find<uint64_t>("stack_start_address", 0x7ffffff0);

        // See if we get an entry point the sub-component says we have to use
        // if not, we will fall back to ELF reading at the core level to work this
        // out
        setInstructionPointer(params.find<uint64_t>("entry_point", 0));
    }

    ~VanadisRISCV64Decoder() {}

    const char*                  getISAName() const override { return "RISCV64"; }
    uint16_t                     countISAIntReg() const override { return options->countISAIntRegisters(); }
    uint16_t                     countISAFPReg() const override { return options->countISAFPRegisters(); }
    const VanadisDecoderOptions* getDecoderOptions() const override { return options; }
    VanadisFPRegisterMode        getFPRegisterMode() const override { return VANADIS_REGISTER_MODE_FP64; }

    void configureApplicationLaunch(
        SST::Output* output, VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, VanadisLoadStoreQueue* lsq,
        VanadisELFInfo* elf_info, SST::Params& params) override
    {

        output->verbose(CALL_INFO, 16, 0, "Application Startup Processing:\n");

        const uint32_t arg_count = params.find<uint32_t>("argc", 1);
        const uint32_t env_count = params.find<uint32_t>("env_count", 0);

        char*                 arg_name = new char[32];
        std::vector<uint8_t>  arg_data_block;
        std::vector<uint64_t> arg_start_offsets;

        for ( uint32_t arg = 0; arg < arg_count; ++arg ) {
            snprintf(arg_name, 32, "arg%" PRIu32 "", arg);
            std::string arg_value = params.find<std::string>(arg_name, "");

            if ( "" == arg_value ) {
                if ( 0 == arg ) {
                    arg_value = elf_info->getBinaryPath();
                    output->verbose(CALL_INFO, 8, 0, "--> auto-set \"%s\" to \"%s\"\n", arg_name, arg_value.c_str());
                }
                else {
                    output->fatal(
                        CALL_INFO, -1,
                        "Error - unable to find argument %s, value is empty "
                        "string which is not allowed in Linux.\n",
                        arg_name);
                }
            }

            output->verbose(CALL_INFO, 16, 0, "--> Found %s = \"%s\"\n", arg_name, arg_value.c_str());

            uint8_t* arg_value_ptr = (uint8_t*)&arg_value.c_str()[0];

            // Record the start
            arg_start_offsets.push_back((uint64_t)arg_data_block.size());

            for ( size_t j = 0; j < arg_value.size(); ++j ) {
                arg_data_block.push_back(arg_value_ptr[j]);
            }

            arg_data_block.push_back((uint8_t)('\0'));
        }
        delete[] arg_name;

        char*                 env_var_name = new char[32];
        std::vector<uint8_t>  env_data_block;
        std::vector<uint64_t> env_start_offsets;

        for ( uint32_t env_var = 0; env_var < env_count; ++env_var ) {
            snprintf(env_var_name, 32, "env%" PRIu32 "", env_var);
            std::string env_value = params.find<std::string>(env_var_name, "");

            if ( "" == env_value ) {
                output->fatal(
                    CALL_INFO, -1,
                    "Error - unable to find a value for %s, value is empty "
                    "or non-existent which is not allowed.\n",
                    env_var_name);
            }

            output->verbose(CALL_INFO, 16, 0, "--> Found %s = \"%s\"\n", env_var_name, env_value.c_str());

            uint8_t* env_value_ptr = (uint8_t*)&env_value.c_str()[0];

            // Record the start
            env_start_offsets.push_back((uint64_t)env_data_block.size());

            for ( size_t j = 0; j < env_value.size(); ++j ) {
                env_data_block.push_back(env_value_ptr[j]);
            }
            env_data_block.push_back((uint8_t)('\0'));
        }
        delete[] env_var_name;

        std::vector<uint8_t> phdr_data_block;

        for ( int i = 0; i < elf_info->getProgramHeaderEntryCount(); ++i ) {
            const VanadisELFProgramHeaderEntry* nxt_entry = elf_info->getProgramHeader(i);

            vanadis_vec_copy_in<int>(phdr_data_block, (int)nxt_entry->getHeaderTypeNumber());
            vanadis_vec_copy_in<int>(phdr_data_block, (int)nxt_entry->getSegmentFlags());
            vanadis_vec_copy_in<uint64_t>(phdr_data_block, (uint64_t)nxt_entry->getImageOffset());
            vanadis_vec_copy_in<uint64_t>(phdr_data_block, (uint64_t)nxt_entry->getVirtualMemoryStart());
            // Physical address - just ignore this for now
            vanadis_vec_copy_in<uint64_t>(phdr_data_block, (uint64_t)nxt_entry->getPhysicalMemoryStart());
            vanadis_vec_copy_in<uint64_t>(phdr_data_block, (uint64_t)nxt_entry->getHeaderImageLength());
            vanadis_vec_copy_in<uint64_t>(phdr_data_block, (uint64_t)nxt_entry->getHeaderMemoryLength());
            vanadis_vec_copy_in<uint64_t>(phdr_data_block, (uint64_t)nxt_entry->getAlignment());
        }

        if ( elf_info->getEndian() != VANADIS_LITTLE_ENDIAN ) {
            output->fatal(
                CALL_INFO, -1,
                "Error: binary executable ELF information shows this "
                "binary was not compiled for little-endian processors.\n");
        }

        const uint64_t phdr_address = params.find<uint64_t>("program_header_address", 0x60000000);

        std::vector<uint8_t> random_values_data_block;

        for ( int i = 0; i < 16; ++i ) {
            random_values_data_block.push_back(rand() % 255);
        }

        const uint64_t rand_values_address = phdr_address + phdr_data_block.size() + 64;
		  const char* exe_path = elf_info->getBinaryPath();

		  for( int i = 0; i < std::strlen(exe_path); ++i) {
				random_values_data_block.push_back(exe_path[i]);
		  }

		  random_values_data_block.push_back('\0');

        std::vector<uint8_t> aux_data_block;

        // AT_EXECFD (file descriptor of the executable)
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_EXECFD);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 4);

        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_PHDR);
        vanadis_vec_copy_in<int64_t>(aux_data_block, (int)phdr_address);

        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_PHENT);
        vanadis_vec_copy_in<int64_t>(aux_data_block, (int)elf_info->getProgramHeaderEntrySize());

        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_PHNUM);
        vanadis_vec_copy_in<int64_t>(aux_data_block, (int)elf_info->getProgramHeaderEntryCount());

        // System page size
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_PAGESZ);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 4096);

        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_ENTRY);
        vanadis_vec_copy_in<int64_t>(aux_data_block, (int)elf_info->getEntryPoint());

        // AT_BASE (base address loaded into)
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_BASE);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 0);

        // AT_FLAGS
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_FLAGS);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 0);

        // AT_HWCAP
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_HWCAP);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 0);

        // AT_CLKTCK (Clock Tick Resolution)
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_CLKTCK);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 100);

        // Not ELF
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_NOTELF);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 0);

        // Real UID
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_UID);
        vanadis_vec_copy_in<int64_t>(aux_data_block, (int64_t)getuid());

        // Effective UID
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_EUID);
        vanadis_vec_copy_in<int64_t>(aux_data_block, (int64_t)geteuid());

        // Real GID
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_GID);
        vanadis_vec_copy_in<int64_t>(aux_data_block, (int64_t)getgid());

        // Effective GID
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_EGID);
        vanadis_vec_copy_in<int64_t>(aux_data_block, (int64_t)getegid());

        // D-Cache Line Size
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_DCACHEBSIZE);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 64);

        // I-Cache Line Size
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_ICACHEBSIZE);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 64);

        // AT_SECURE?
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_SECURE);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 0);

        // AT_RANDOM - 16 bytes of random stuff
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_RANDOM);
        vanadis_vec_copy_in<int64_t>(aux_data_block, rand_values_address);

        // AT_EXEFN - path to full executable
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_EXECFN);
        vanadis_vec_copy_in<int64_t>(aux_data_block, rand_values_address + 16);

        // End the Auxillary vector
        vanadis_vec_copy_in<int64_t>(aux_data_block, VANADIS_AT_NULL);
        vanadis_vec_copy_in<int64_t>(aux_data_block, 0);

		  // How many entries do we have, divide by 8 because we are running on a 64b system
        const int aux_entry_count = aux_data_block.size() / 8;

        // Per RISCV Assembly Programemr's handbook, register x2 is for stack
        // pointer
        const int16_t sp_phys_reg = isa_tbl->getIntPhysReg(2);

        output->verbose(CALL_INFO, 16, 0, "-> Argument Count:                       %" PRIu32 "\n", arg_count);
        output->verbose(
            CALL_INFO, 16, 0, "---> Data Size for items:                %" PRIu32 "\n",
            (uint32_t)arg_data_block.size());
        output->verbose(
            CALL_INFO, 16, 0, "-> Environment Variable Count:           %" PRIu32 "\n",
            (uint32_t)env_start_offsets.size());
        output->verbose(
            CALL_INFO, 16, 0, "---> Data size for items:                %" PRIu32 "\n",
            (uint32_t)env_data_block.size());
        output->verbose(
            CALL_INFO, 16, 0, "---> Data size of aux-vector:            %" PRIu32 "\n",
            (uint32_t)aux_data_block.size());
        output->verbose(CALL_INFO, 16, 0, "---> Aux entry count:                    %d\n", aux_entry_count);
        // output->verbose(CALL_INFO, 16, 0, "-> Full Startup Data Size: %" PRIu32
        // "\n", (uint32_t) stack_data.size() );
        output->verbose(CALL_INFO, 16, 0, "-> Stack Pointer (r29) maps to phys-reg: %" PRIu16 "\n", sp_phys_reg);
        output->verbose(
            CALL_INFO, 16, 0, "-> Setting SP to (not-aligned):          %" PRIu64 " / 0x%0llx\n", start_stack_address,
            start_stack_address);

        uint64_t arg_env_space_needed = 1 + arg_count + 1 + env_count + 1 + aux_entry_count;
        uint64_t arg_env_space_and_data_needed =
            (arg_env_space_needed * 8) + arg_data_block.size() + env_data_block.size() + aux_data_block.size();

        uint64_t       aligned_start_stack_address = (start_stack_address - arg_env_space_and_data_needed);
        const uint64_t padding_needed              = (aligned_start_stack_address % 64);
        aligned_start_stack_address                = aligned_start_stack_address - padding_needed;

        output->verbose(
            CALL_INFO, 16, 0,
            "Aligning stack address to 64 bytes (%" PRIu64 " - %" PRIu64 " - padding: %" PRIu64 " = %" PRIu64
            " / 0x%0llx)\n",
            start_stack_address, (uint64_t)arg_env_space_and_data_needed, padding_needed, aligned_start_stack_address,
            aligned_start_stack_address);

        start_stack_address = aligned_start_stack_address;

        // Allocate 64 zeros for now
        std::vector<uint8_t> stack_data;

        const uint64_t arg_env_data_start = start_stack_address + (arg_env_space_needed * 8);

        output->verbose(CALL_INFO, 16, 0, "-> Setting start of stack data to:       %" PRIu64 "\n", arg_env_data_start);

		  // Is this 32b or 64b for a 64b environment? (code looks like an int from Linux)
        vanadis_vec_copy_in<uint64_t>(stack_data, arg_count);

        for ( size_t i = 0; i < arg_start_offsets.size(); ++i ) {
            output->verbose(
                CALL_INFO, 16, 0, "--> Setting arg%" PRIu32 " to point to address %" PRIu64 " / 0x%llx\n", (uint32_t)i,
                arg_env_data_start + arg_start_offsets[i], arg_env_data_start + arg_start_offsets[i]);
            vanadis_vec_copy_in<uint64_t>(stack_data, (uint64_t)(arg_env_data_start + arg_start_offsets[i]));
        }

        vanadis_vec_copy_in<uint64_t>(stack_data, 0);

        for ( size_t i = 0; i < env_start_offsets.size(); ++i ) {
            output->verbose(
                CALL_INFO, 16, 0, "--> Setting env%" PRIu32 " to point to address %" PRIu64 " / 0x%llx\n", (uint32_t)i,
                arg_env_data_start + arg_data_block.size() + env_start_offsets[i],
                arg_env_data_start + arg_data_block.size() + env_start_offsets[i]);

            vanadis_vec_copy_in<uint64_t>(
                stack_data, (uint64_t)(arg_env_data_start + arg_data_block.size() + env_start_offsets[i]));
        }

        vanadis_vec_copy_in<uint64_t>(stack_data, 0);

        for ( size_t i = 0; i < aux_data_block.size(); ++i ) {
            stack_data.push_back(aux_data_block[i]);
        }

        for ( size_t i = 0; i < arg_data_block.size(); ++i ) {
            stack_data.push_back(arg_data_block[i]);
        }

        for ( size_t i = 0; i < env_data_block.size(); ++i ) {
            stack_data.push_back(env_data_block[i]);
        }

        for ( size_t i = 0; i < padding_needed; ++i ) {
            vanadis_vec_copy_in<uint8_t>(stack_data, (uint8_t)0);
        }

        output->verbose(
            CALL_INFO, 16, 0, "-> Pushing %" PRIu64 " bytes to the start of stack (0x%llx) via memory init event..\n",
            (uint64_t)stack_data.size(), start_stack_address);

        uint64_t index = 0;
        while ( index < stack_data.size() ) {
            uint64_t inner_index = 0;

            while ( inner_index < 4 ) {
                // if( index < stack_data.size() ) {
                //   printf("0x%x ", stack_data[index]);
                // }

                index++;
                inner_index++;
            }

            // printf("\n");
        }

        output->verbose(
            CALL_INFO, 16, 0,
            "-> Sending inital write of auxillary vector to memory, "
            "forms basis of stack start (addr: 0x%llx)\n",
            start_stack_address);

        lsq->setInitialMemory(start_stack_address, stack_data);

        output->verbose(
            CALL_INFO, 16, 0,
            "-> Sending initial write of AT_RANDOM values to memory "
            "(0x%llx, len: %" PRIu64 ")\n",
            rand_values_address, (uint64_t)random_values_data_block.size());

        lsq->setInitialMemory(rand_values_address, random_values_data_block);

        output->verbose(
            CALL_INFO, 16, 0,
            "-> Sending initial data for program headers (addr: "
            "0x%llx, len: %" PRIu64 ")\n",
            phdr_address, (uint64_t)phdr_data_block.size());

        lsq->setInitialMemory(phdr_address, phdr_data_block);

        output->verbose(
            CALL_INFO, 16, 0, "-> Setting SP to (64B-aligned):          %" PRIu64 " / 0x%0llx\n", start_stack_address,
            start_stack_address);

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
    uint64_t                     start_stack_address;
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
                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 1, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 1:
                {
                    // LH
                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 2, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 2:
                {
                    // LW
                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 4, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 3:
                {
                    // LD
                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 8, true, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 4:
                {
                    // LBU
                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 1, false, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 5:
                {
                    // LHU
                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 2, false, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                case 6:
                {
                    // LWU
                    bundle->addInstruction(new VanadisLoadInstruction(
                        ins_address, hw_thr, options, rs1, simm64, rd, 4, false, MEM_TRANSACTION_NONE,
                        LOAD_INT_REGISTER));
                    decode_fault = false;
                } break;
                }
            } break;
            case 0x7:
            {
                // Floating point load
            } break;
            case 0x23:
            {
                // Store data
                processS<int64_t>(ins, op_code, rs1, rs2, func_code3, simm64);

                if ( func_code < 4 ) {
                    // shift to get the power of 2 number of bytes to store
                    const uint32_t store_bytes = 1 << func_code;
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

                    bundle->addInstruction(new VanadisAddImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
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

							output->verbose(CALL_INFO, 16, 0, "------> func_code6 = %" PRIu32 " / shift = %" PRIu32 " (0x%lx)\n", func_code6, shift_by, shift_by);

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

							output->verbose(CALL_INFO, 16, 0, "------> func_code6 = %" PRIu32 " / shift = %" PRIu32 " (0x%lx)\n", func_code6, shift_by, shift_by);

                    switch ( func_code6 ) {
                    case 0x0:
                    {
								  output->verbose(CALL_INFO, 16, 0, "--------> SRLI %" PRIu16 " <- %" PRIu16 " << %" PRIu32 "\n",
										rd, rs1, shift_by);
                        bundle->addInstruction(
                            new VanadisShiftRightLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, shift_by));
                        decode_fault = false;
                    } break;
						  case 0x40000000:
                    {
								  output->verbose(CALL_INFO, 16, 0, "--------> SRAI %" PRIu16 " <- %" PRIu16 " << %" PRIu32 "\n",
										rd, rs1, shift_by);
                        bundle->addInstruction(
                            new VanadisShiftRightArithmeticImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, static_cast<uint64_t>(rs2)));
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

					 output->verbose(CALL_INFO, 16, 0, "-----> decode R-type, func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n", func_code3, func_code7);

                switch ( func_code3 ) {
                case 0:
                {
                    switch ( func_code7 ) {
						  case 0:
							{
								output->verbose(CALL_INFO, 16, 0, "-------> ADD %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n",
									rd, rs1, rs2);
							   // ADD
								bundle->addInstruction(new VanadisAddInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64, true>(
									ins_address, hw_thr, options, rd, rs1, rs2));
								decode_fault = false;
							} break;
                    case 1:
                    {
                        // MUL
                        // TODO - check register ordering
								output->verbose(CALL_INFO, 16, 0, "-------> MUL %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n",
									rd, rs1, rs2);

                        bundle->addInstruction(
                            new VanadisMultiplyInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
							case 0x20:
							{
								output->verbose(CALL_INFO, 16, 0, "-------> SUB %" PRIu16 " <- %" PRIu16 " - %" PRIu16 "\n",
									rd, rs1, rs2);
							   // SUB
								bundle->addInstruction(new VanadisSubInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
									ins_address, hw_thr, options, rd, rs1, rs2, false));
								decode_fault = false;
							} break;
                    };
                } break;
                case 1:
                {
                    switch ( func_code7 ) {
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
                    case 1:
                    {
                        // MULHSU
                        // NOT SURE WHAT THIS IS?
                    } break;
                    };
                } break;
                case 4:
                {
                    switch ( func_code7 ) {
                    case 1:
                    {
                        // DIV
                        bundle->addInstruction(
                            new VanadisDivideInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64, true>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 5:
                {
                    switch ( func_code7 ) {
                    case 1:
                    {
                        // DIVU
                        bundle->addInstruction(
                            new VanadisDivideInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64, false>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    };
                } break;
                case 6:
                {
                    // TODO - REM INSTRUCTION
						 switch( func_code7 ) {
						case 0x0:
							{
							  // OR
								output->verbose(CALL_INFO, 16, 0, "-----> OR %" PRIu16 " <- %" PRIu16 " | %" PRIu16 "\n",
									rd, rs1, rs2);

								bundle->addInstruction(new VanadisOrInstruction(ins_address, hw_thr, options, rd, rs1, rs2));
								decode_fault = false;
							} break;
						}
                } break;
                case 7:
                {
                    // TODO - REMU INSTRUCTION
                } break;
                };
            } break;
            case 0x37:
            {
                // LUI
                processU<int64_t>(ins, op_code, rd, simm64);

                bundle->addInstruction(new VanadisSetRegisterInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                    ins_address, hw_thr, options, rd, simm64));
                decode_fault = false;
            } break;
            case 0x17:
            {
                // AUIPC
                processU<int64_t>(ins, op_code, rd, simm64);

                bundle->addInstruction(new VanadisPCAddImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                    ins_address, hw_thr, options, rd, simm64));
                decode_fault = false;
            } break;

				case 0x1B:
				{
					processR(ins, op_code, rd, rs1, rs2, func_code3, func_code7);

                output->verbose(CALL_INFO, 16, 0, "-----> decode R-type, func_code3=%" PRIu32 " / func_code7=%" PRIu32 "\n", func_code3, func_code7);

					 switch(func_code7) {
					 case 0:
						{
							switch(func_code3) {
							case 0x1:
								{
									// RS2 acts as an immediate
									// SLLIW (32bit result generated)
									output->verbose(CALL_INFO, 16, 0, "-------> SLLIW %" PRIu16 " <- %" PRIu16 " << %" PRIu16 "\n",
										rd, rs1, rs2);
									bundle->addInstruction(new VanadisShiftLeftLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
										ins_address, hw_thr, options, rd, rs1, rs2));
									decode_fault = false;
								} break;
							case 0x5:
								{
									// RS2 acts as an immediate
									// SRLIW (32bit result generated)
									output->verbose(CALL_INFO, 16, 0, "-------> SRLIW %" PRIu16 " <- %" PRIu16 " << %" PRIu16 "\n",
										rd, rs1, rs2);
									bundle->addInstruction(new VanadisShiftRightLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
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
						  output->verbose(CALL_INFO, 16, 0, "-----> BEQ %" PRIu16 " == %" PRIu16 " / offset: %" PRId64 "\n",
								rs1, rs2, simm64);
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_EQ>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 1:
                {
                    // BNE
						  output->verbose(CALL_INFO, 16, 0, "-----> BNE %" PRIu16 " != %" PRIu16 " / offset: %" PRId64 " (ip+offset %" PRIu64 ")\n",
								rs1, rs2, simm64, ins_address + simm64);
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_NEQ>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 4:
                {
                    // BLT
						  output->verbose(CALL_INFO, 16, 0, "-----> BLT %" PRIu16 " < %" PRIu16 " / offset: %" PRId64 "\n",
								rs1, rs2, simm64);
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_LT>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 5:
                {
                    // BGE
						  output->verbose(CALL_INFO, 16, 0, "-----> BGE %" PRIu16 " >= %" PRIu16 " / offset: %" PRId64 "\n",
								rs1, rs2, simm64);
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_GTE>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                case 6:
                {
                    // BLTU
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_LT>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT, false));
                    decode_fault = false;
                } break;
                case 7:
                {
                    // BGEU
                    bundle->addInstruction(new VanadisBranchRegCompareInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_GTE>(
                        ins_address, hw_thr, options, 4, rs1, rs2, simm64, VANADIS_NO_DELAY_SLOT, false));
                    decode_fault = false;
                } break;
                };
            } break;
            case 0x73:
            {
                // Syscall/ECALL and EBREAK
                // Control registers

					uint32_t func_code12 = (ins & 0xFFF00000);

					switch(func_code12) {
					case 0x0:
						{
							output->verbose(CALL_INFO, 16, 0, "------> ECALL/SYSCALL\n");
							bundle->addInstruction(new VanadisSysCallInstruction(ins_address, hw_thr, options));
							decode_fault = false;
						} break;
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
                            new VanadisAddInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64, true>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x1:
                    {
                        // MULW
                        // TODO - check register ordering
                        bundle->addInstruction(
                            new VanadisMultiplyInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                    } break;
                    case 0x20:
                    {
                        // SUBW
                        // TODO - check register ordering
                        bundle->addInstruction(new VanadisSubInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
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
                            new VanadisShiftLeftLogicalInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
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
                            new VanadisDivideInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64, true>(
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
                            new VanadisShiftRightLogicalInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x1:
                    {
                        // DIVUW
                        bundle->addInstruction(
                            new VanadisDivideInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64, false>(
                                ins_address, hw_thr, options, rd, rs1, rs2));
                        decode_fault = false;
                    } break;
                    case 0x20:
                    {
                        // SRAW
                        // TODO - check register ordering
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
                    case 0x1:
                    {
                        // REMW
                        // TODO - Implement
                    } break;
                    };
                } break;
                case 7:
                {
                    switch ( func_code7 ) {
                    case 0x1:
                    {
                        // REMUW
                        // TODO - Implement
                    } break;
                    }
                }; break;
                };
            } break;
            case 0xF:
            {
                // Fences (FENCE.I Zifencei)
            }
            case 0x2F:
            {
                // Atomic operations (A extension)
            } break;
            case 0x27:
            {
                // Floating point store
            } break;
            case 0x53:
            {
                // floating point arithmetic
            } break;
            case 0x43:
            {
                // FMADD
            } break;
            case 0x47:
            {
                // FMSUB
            } break;
            case 0x4B:
            {
                // FNMSUB
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
	                    uint16_t rvc_rd     = expand_rvc_int_register(extract_rs2_rvc(ins));

							   uint32_t imm_3 = (ins & 0x20) >> 2;
								uint32_t imm_2 = (ins & 0x40) >> 4;
								uint32_t imm_96 = (ins & 0x780) >> 1;
								uint32_t imm_54 = (ins & 0x1800) >> 7;

								uint32_t imm = (imm_2 | imm_3 | imm_54 | imm_96);

								output->verbose(CALL_INFO, 16, 0, "-------> RVC ADDI4SPN %" PRIu16 " <- r2 (stack-ptr) + %" PRIu32 "\n", rvc_rd, imm);

								bundle->addInstruction( new VanadisAddImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
									ins_address, hw_thr, options, rvc_rd, 2, imm));
								decode_fault = false;
                    }
                } break;
                case 0x2000:
                {
                    // FLD
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
                    uint64_t imm_address = extract_uimm_rcv_t2(ins);
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
                } break;
                case 0xC000:
                {
                    // SW
                    uint16_t rvc_rs2  = expand_rvc_int_register(extract_rs2_rvc(ins));
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
                    uint16_t rvc_rs2  = expand_rvc_int_register(extract_rs2_rvc(ins));
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
                    uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

                    if ( 0 == rvc_rs1 ) {
                        // This is a no-op?
                        output->verbose(CALL_INFO, 16, 0, "-----> creates a no-op.\n");
                        bundle->addInstruction(
                            new VanadisAddImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
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
                            new VanadisAddImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rvc_rs1, rvc_rs1, imm));
                        decode_fault = false;
                    }
                } break;
                case 0x2000:
                {
                    // ADDIW
                    uint16_t rd     = static_cast<uint16_t>((ins & 0xF80) >> 7);
                    uint32_t imm_40 = (ins & 0x7C) >> 2;
                    uint32_t imm_5  = (ins & 0x1000) >> 6;

                    int64_t imm = (imm_40 | imm_5);

                    if ( imm_5 != 0 ) { imm |= 0xFFFFFFFFFFFFFFC0; }

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC ADDIW  reg: %" PRIu16 ", imm=%" PRId64 "\n", rd, imm);

						  // This really is a W-clipping instruction, so make INT32
							bundle->addInstruction(new VanadisAddImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
								ins_address, hw_thr, options, rd, rd, imm));
						decode_fault = false;
                } break;
                case 0x4000:
                {
                    // LI
                    uint16_t rd     = static_cast<uint16_t>((ins & 0xF80) >> 7);
                    uint32_t imm_40 = (ins & 0x7C) >> 2;
                    uint32_t imm_5  = (ins & 0x1000) >> 6;

                    int64_t imm = (imm_40 | imm_5);

                    if ( imm_5 != 0 ) { imm |= 0xFFFFFFFFFFFFFFC0; }

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC load imediate (LI) reg: %" PRIu16 ", imm=%" PRId64 "\n", rd, imm);

                    bundle->addInstruction(
                        new VanadisSetRegisterInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                            ins_address, hw_thr, options, rd, imm));
                    decode_fault = false;
                } break;
                case 0x6000:
                {
                    // ADDI16SP / LUI
                    uint16_t rd = static_cast<uint16_t>((ins & 0xF80) >> 7);

                    if ( rd != 2 ) {
                        // LUI
                        uint32_t imm_1612 = (ins & 0x7C) << 9;
                        uint32_t imm_17   = (ins & 0x1000) << 4;

                        int64_t imm = (imm_17 | imm_1612);

                        output->verbose(
                            CALL_INFO, 16, 0,
                            "-----> RVC load imediate (LUI) reg: %" PRIu16 ", imm=%" PRId64 " (0x%llx)\n", rd, imm,
                            imm);

                        bundle->addInstruction(
                            new VanadisSetRegisterInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                ins_address, hw_thr, options, rd, imm));
                        decode_fault = false;
                    }
                    else {
                        // ADDI16SP
                        output->verbose(CALL_INFO, 16, 0, "----> RVC ADDI16SP\n");

                        uint32_t imm_5  = (ins & 0x4) << 3;
                        uint32_t imm_87 = (ins & 0x18) << 4;
                        uint32_t imm_6  = (ins & 0x20) << 1;
                        uint32_t imm_4  = (ins & 0x40) >> 2;
                        uint32_t imm_9  = (ins & 0x1000) >> 3;

                        int64_t imm = imm_5 | imm_87 | imm_6 | imm_4 | imm_9;

                        if ( imm_9 != 0 ) { imm |= 0xFFFFFFFFFFFFFC00; }

                        output->verbose(CALL_INFO, 16, 0, "-----> RVC ADDI16SP r2 <- %" PRId64 "\n", imm);

                        bundle->addInstruction(
                            new VanadisAddImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
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

								if( imm_5 != 0 ) {
									imm |= 0xFFFFFFFFFFFFFFC0;
								}

                        uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

								output->verbose(CALL_INFO, 16, 0, "--------> RVC ANDI %" PRIu16 " = %" PRIu16 " & %" PRIu64 " (0x%llx)\n",
									rvc_rs1, rvc_rs1, imm, imm);

								bundle->addInstruction(
									new VanadisAndImmInstruction(ins_address, hw_thr, options, rvc_rs1, rvc_rs1, imm));
								decode_fault = false;
                    } break;
                    case 0xC00:
                    {
                        // Arith
                        uint16_t rvc_rd  = expand_rvc_int_register(extract_rs2_rvc(ins));
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
                                    rvc_rd, rvc_rd, rvc_rs1);
                                bundle->addInstruction(
                                    new VanadisSubInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                                        ins_address, hw_thr, options, rvc_rd, rvc_rd, rvc_rs1, true));
                                decode_fault = false;
                            } break;
                            case 0x20:
                            {
                                // XOR
                                output->verbose(
                                    CALL_INFO, 16, 0, "--------> RVC XOR %" PRIu16 " <- %" PRIu16 " - %" PRIu16 "\n",
                                    rvc_rd, rvc_rd, rvc_rs1);
                                bundle->addInstruction(
                                    new VanadisXorInstruction(ins_address, hw_thr, options, rvc_rd, rvc_rd, rvc_rs1));
                                decode_fault = false;
                            } break;
                            case 0x40:
                            {
                                // OR
                                output->verbose(
                                    CALL_INFO, 16, 0, "--------> RVC OR %" PRIu16 " <- %" PRIu16 " - %" PRIu16 "\n",
                                    rvc_rd, rvc_rd, rvc_rs1);
                                bundle->addInstruction(
                                    new VanadisOrInstruction(ins_address, hw_thr, options, rvc_rd, rvc_rd, rvc_rs1));
                                decode_fault = false;
                            } break;
                            case 0x60:
                            {
                                // AND
                                output->verbose(
                                    CALL_INFO, 16, 0, "--------> RVC AND %" PRIu16 " <- %" PRIu16 " - %" PRIu16 "\n",
                                    rvc_rd, rvc_rd, rvc_rs1);
                                bundle->addInstruction(
                                    new VanadisAndInstruction(ins_address, hw_thr, options, rvc_rd, rvc_rd, rvc_rs1));
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
                                output->verbose(CALL_INFO, 16, 0, "------> RVC SUBW\n");

                            } break;
                            case 0x20:
                            {
                                // ADDW
		                           uint16_t rvc_rd  = expand_rvc_int_register(extract_rs2_rvc(ins));
      		                    uint16_t rvc_rs1 = expand_rvc_int_register(extract_rs1_rvc(ins));

										  output->verbose(CALL_INFO, 16, 0, "------> RVC ADDW %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n",
											 rvc_rs1, rvc_rs1, rvc_rd);

										  bundle->addInstruction(new VanadisAddInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32, true>(ins_address,
												hw_thr, options, rvc_rs1, rvc_rs1, rvc_rd));
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

                    if ( imm_8 != 0 ) { imm_final |= 0xFFFFFFFFFFFFF00; }

                    output->verbose(
                        CALL_INFO, 16, 0, "----> decode RVC BEQZ %" PRIu16 " jump to: 0x%llx\n", rvc_rs1,
                        ins_address + imm_final);

                    bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_EQ>(
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
                        CALL_INFO, 16, 0, "----> decode RVC BNEZ %" PRIu16 " jump to: 0x%llx\n", rvc_rs1,
                        ins_address + imm_final);

                    bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<
                                           VanadisRegisterFormat::VANADIS_FORMAT_INT64, REG_COMPARE_NEQ>(
                        ins_address, hw_thr, options, 2, rvc_rs1, 0, imm_final, VANADIS_NO_DELAY_SLOT));
                    decode_fault = false;
                } break;
                }
            } break;
            case 0x2:
            {
                const uint32_t c_func_code = ins & 0xE000;

                switch ( c_func_code ) {
                case 0x0:
                {
                    // SLLI
                    uint32_t shamt_5  = (ins & 0x1000) >> 7;
                    uint32_t shamt_40 = (ins & 0x7C) >> 2;

                    uint64_t shift_by = shamt_5 | shamt_40;

                    uint16_t rvc_rs1 = (ins & 0xF80) >> 7;

                    output->verbose(
                        CALL_INFO, 16, 0, "--------> RVC SLLI %" PRIu16 " = %" PRIu16 " >> %" PRIu64 " (0x%llx)\n",
                        rvc_rs1, rvc_rs1, shift_by, shift_by);
                    bundle->addInstruction(
                        new VanadisShiftLeftLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64>(
                            ins_address, hw_thr, options, rvc_rs1, rvc_rs1, shift_by));
                    decode_fault = false;

                } break;
                case 0x2000:
                {
                    // FLDSP
                } break;
                case 0x4000:
                {
                    // LWSP
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
                                    new VanadisAddInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64, true>(
                                        ins_address, hw_thr, options, static_cast<uint16_t>(c_func_12_rs1),
                                        static_cast<uint16_t>(c_func_12_rs2), options->getRegisterIgnoreWrites()));
                                decode_fault = false;
                            }
                        }
                    }
                    else {
                        // ADD
                        output->verbose(
                            CALL_INFO, 16, 0,
                            "--------> (generates) RVC ADD %" PRIu16 " <- %" PRIu16 " + %" PRIu16 "\n", c_func_12_rs1,
                            c_func_12_rs1, c_func_12_rs2);

                        bundle->addInstruction(
                            new VanadisAddInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT64, true>(
                                ins_address, hw_thr, options, static_cast<uint16_t>(c_func_12_rs1),
                                static_cast<uint16_t>(c_func_12_rs1), static_cast<uint16_t>(c_func_12_rs2)));
                        decode_fault = false;
                    }
                } break;
                case 0xA000:
                {
                    // FSDSP
                } break;
                case 0xC000:
                {
                    // SWSP
                    uint16_t rvc_src = (ins & 0x7C) >> 2;
                    ;

                    uint32_t offset_52 = (ins & 0x1E00) >> 7;
                    uint32_t offset_76 = (ins & 0x180) >> 1;

                    uint32_t offset = offset_76 | offset_52;

                    output->verbose(
                        CALL_INFO, 16, 0, "-----> RVC SWSP %" PRIu16 " -> %" PRIu16 " + %" PRIu64 "\n", rvc_src, 2,
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
                        CALL_INFO, 16, 0, "-----> RVC SDSP %" PRIu16 " -> %" PRIu16 " + %" PRIu64 "\n", rvc_src, 2,
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
            if ( decode_fault ) { output->fatal(CALL_INFO, -1, "STOP\n"); }
        }

        //		  if(decode_fault) {
        //				bundle->addInstruction(new VanadisInstructionDecodeFault(ins_address, hw_thr,
        // options));
        //			}
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
            sign_extend12(((ins_i32 & VANADIS_RISCV_RD_MASK) >> 6) | ((ins_i32 & VANADIS_RISCV_FUNC7_MASK) >> 24)));
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

        const int32_t ins_i32  = static_cast<int32_t>(ins);

		  const int32_t imm_12   = (ins_i32 & 0x80000000) >> 19;
        const int32_t imm_11   = (ins_i32 & 0x80) << 4;
		  const int32_t imm_105  = (ins_i32 & 0x7E000000) >> 20;
		  const int32_t imm_41   = (ins_i32 & 0xF00) >> 7;

		  int32_t imm_tmp = imm_12 | imm_11 | imm_105 | imm_41;

		  if( imm_12 != 0 ) {
			imm_tmp |= 0xFFFFF000;
		  }

        imm = static_cast<T>(imm_tmp);
    }
};
} // namespace Vanadis
} // namespace SST

#endif
