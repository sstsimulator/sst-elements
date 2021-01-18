
#ifndef _H_VANADIS_MIPS_DECODER
#define _H_VANADIS_MIPS_DECODER

#include "decoder/vdecoder.h"
#include "decoder/vauxvec.h"
#include "inst/isatable.h"
#include "vinsloader.h"
#include "inst/vinstall.h"
#include "os/vmipscpuos.h"

#include "util/vdatacopy.h"

#include <list>

#include <unistd.h>
#include <sys/types.h>

#define MIPS_REG_ZERO		  0
#define MIPS_REG_LO		  32
#define MIPS_REG_HI		  33

#define MIPS_FP_VER_REG           32
#define MIPS_FP_STATUS_REG        33

#define MIPS_OP_MASK              0xFC000000
#define MIPS_RS_MASK              0x3E00000
#define MIPS_RT_MASK              0x1F0000
#define MIPS_RD_MASK              0xF800

#define MIPS_FD_MASK              0x7C0
#define MIPS_FS_MASK              0xF800
#define MIPS_FT_MASK              0x1F0000
#define MIPS_FR_MASK              0x3E00000

#define MIPS_ADDR_MASK            0x7FFFFFF
#define MIPS_J_ADDR_MASK          0x3FFFFFF
#define MIPS_J_UPPER_MASK         0xF0000000
#define MIPS_IMM_MASK             0xFFFF
#define MIPS_SHFT_MASK            0x7C0
#define MIPS_FUNC_MASK       	  0x3F

#define MIPS_SPEC_OP_SPECIAL3     0x7c000000

#define MIPS_SPECIAL_OP_MASK      0x7FF

#define MIPS_SPEC_OP_MASK_ADD     0x20
#define MIPS_SPEC_OP_MASK_ADDU    0x21
#define MIPS_SPEC_OP_MASK_AND     0x24

#define MIPS_SPEC_OP_MASK_ANDI    0x30000000
#define MIPS_SPEC_OP_MASK_ORI     0x34000000
#define MIPS_SPEC_OP_MASK_REGIMM  0x04000000
#define MIPS_SPEC_OP_MASK_BGEZAL  0x00110000
#define MIPS_SPEC_OP_MASK_BGTZ    0x1C000000
#define MIPS_SPEC_OP_MASK_LUI     0x3C000000
#define MIPS_SPEC_OP_MASK_ADDIU   0x24000000
#define MIPS_SPEC_OP_MASK_LB      0x80000000
#define MIPS_SPEC_OP_MASK_LBU     0x90000000
#define MIPS_SPEC_OP_MASK_LL      0xC0000000
#define MIPS_SPEC_OP_MASK_LW      0x8C000000
#define MIPS_SPEC_OP_MASK_LWL     0x88000000
#define MIPS_SPEC_OP_MASK_LWR     0x98000000
#define MIPS_SPEC_OP_MASK_LHU     0x94000000
#define MIPS_SPEC_OP_MASK_SB      0xA0000000
#define MIPS_SPEC_OP_MASK_SC      0xE0000000
#define MIPS_SPEC_OP_MASK_SH      0xA4000000
#define MIPS_SPEC_OP_MASK_SW      0xAC000000
#define MIPS_SPEC_OP_MASK_SWL     0xA8000000
#define MIPS_SPEC_OP_MASK_SWR     0xB8000000
#define MIPS_SPEC_OP_MASK_BEQ     0x10000000
#define MIPS_SPEC_OP_MASK_BNE     0x14000000
#define MIPS_SPEC_OP_MASK_BLEZ    0x18000000
#define MIPS_SPEC_OP_MASK_SLTI    0x28000000
#define MIPS_SPEC_OP_MASK_SLTIU   0x2C000000
#define MIPS_SPEC_OP_MASK_JAL     0x0C000000
#define MIPS_SPEC_OP_MASK_J       0x08000000
#define MIPS_SPEC_OP_MASK_COP1	  0x44000000
#define MIPS_SPEC_OP_MASK_XORI    0x38000000

#define MIPS_SPEC_OP_MASK_SFP32   0xE4000000
#define MIPS_SPEC_OP_MASK_LFP32   0xC4000000

#define MIPS_SPEC_OP_MASK_BLTZ    0x0
#define MIPS_SPEC_OP_MASK_BGEZ    0x10000
#define MIPS_SPEC_OP_MASK_BREAK   0x0D
#define MIPS_SPEC_OP_MASK_DADD    0x2C
#define MIPS_SPEC_OP_MASK_DADDU   0x2D
#define MIPS_SPEC_OP_MASK_DIV     0x1A
#define MIPS_SPEC_OP_MASK_DIVU    0x1B
#define MIPS_SPEC_OP_MASK_DDIV    0x1E
#define MIPS_SPEC_OP_MASK_DDIVU   0x1F
#define MIPS_SPEC_OP_MASK_DMULT	  0x1C
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
#define MIPS_SPEC_OP_MASK_SLT	  0x2A
#define MIPS_SPEC_OP_MASK_SLTU    0x2B
#define MIPS_SPEC_OP_MASK_SRA	  0x03
#define MIPS_SPEC_OP_MASK_SRAV    0x07
#define MIPS_SPEC_OP_MASK_SRL	  0x02
#define MIPS_SPEC_OP_MASK_SRLV    0x06
#define MIPS_SPEC_OP_MASK_SUB	  0x22
#define MIPS_SPEC_OP_MASK_SUBU    0x23
#define MIPS_SPEC_OP_MASK_SYSCALL 0x0C
#define MIPS_SPEC_OP_MASK_SYNC    0x0F
#define MIPS_SPEC_OP_MASK_XOR     0x26

#define MIPS_SPEC_COP_MASK_CF	    0x2
#define MIPS_SPEC_COP_MASK_CT       0x6
#define MIPS_SPEC_COP_MASK_MFC      0x0
#define MIPS_SPEC_COP_MASK_MTC	    0x4
#define MIPS_SPEC_COP_MASK_MOV	    0x6
#define MIPS_SPEC_COP_MASK_CVTS     0x20
#define MIPS_SPEC_COP_MASK_CVTD     0x21
#define MIPS_SPEC_COP_MASK_CVTW     0x24
#define MIPS_SPEC_COP_MASK_MUL      0x2
#define MIPS_SPEC_COP_MASK_ADD      0x0
#define MIPS_SPEC_COP_MASK_SUB      0x1
#define MIPS_SPEC_COP_MASK_DIV      0x3

#define MIPS_SPEC_COP_MASK_CMP_LT   0x3C
#define MIPS_SPEC_COP_MASK_CMP_LTE  0x3E
#define MIPS_SPEC_COP_MASK_CMP_EQ   0x32

namespace SST {
namespace Vanadis {

class VanadisMIPSDecoder : public VanadisDecoder {
public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		VanadisMIPSDecoder,
		"vanadis",
		"VanadisMIPSDecoder",
		SST_ELI_ELEMENT_VERSION(1,0,0),
		"Implements a MIPS-compatible decoder for Vanadis CPU processing.",
		SST::Vanadis::VanadisDecoder )

	SST_ELI_DOCUMENT_PARAMS(
		{ "decode_max_ins_per_cycle", 		"Maximum number of instructions that can be decoded and issued per cycle"  				},
		{ "uop_cache_entries",                  "Number of micro-op cache entries, this corresponds to ISA-level instruction counts."  			},
		{ "predecode_cache_entries",            "Number of cache lines that a cached prior to decoding (these support loading from cache prior to decode)" },
		{ "stack_start_address",                "Sets the start of the stack and dynamic program segments" }
		)

	VanadisMIPSDecoder( ComponentId_t id, Params& params ) :
		VanadisDecoder( id, params ) {

		// 32 int + hi/lo (2) = 34
		// 32 fp + ver + status (2) = 34
		// reg-2 is for sys-call codes
		// plus 2 for LO/HI registers in INT
		options = new VanadisDecoderOptions( (uint16_t) 0, 34, 34, 2, VANADIS_REGISTER_MODE_FP32 );
		max_decodes_per_cycle      = params.find<uint16_t>("decode_max_ins_per_cycle",  2);

		// MIPS default is 0x7fffffff according to SYS-V manual
		start_stack_address        = params.find<uint64_t>("stack_start_address", 0x7ffffff0);

		// See if we get an entry point the sub-component says we have to use
		// if not, we will fall back to ELF reading at the core level to work this out
		setInstructionPointer( params.find<uint64_t>("entry_point", 0) );

		haltOnDecodeZero = true;
	}

	~VanadisMIPSDecoder() {}

	virtual const char* getISAName() const { return "MIPS"; }
	virtual uint16_t countISAIntReg() const { return options->countISAIntRegisters(); }
	virtual uint16_t countISAFPReg() const { return options->countISAFPRegisters(); }
	virtual const VanadisDecoderOptions* getDecoderOptions() const { return options; }

	virtual VanadisFPRegisterMode getFPRegisterMode() const { return VANADIS_REGISTER_MODE_FP32; }

	virtual void configureApplicationLaunch( SST::Output* output, VanadisISATable* isa_tbl,
		VanadisRegisterFile* regFile, VanadisLoadStoreQueue* lsq,
		VanadisELFInfo* elf_info, SST::Params& params ) {

		output->verbose(CALL_INFO, 16, 0, "Application Startup Processing:\n");

		const uint32_t arg_count = params.find<uint32_t>("argc", 1);
		const uint32_t env_count = params.find<uint32_t>("env_count", 0);

		char* arg_name = new char[32];
		std::vector<uint8_t> arg_data_block;
		std::vector<uint64_t> arg_start_offsets;

		for( uint32_t arg = 0; arg < arg_count; ++arg ) {
			snprintf( arg_name, 32, "arg%" PRIu32 "", arg );
			std::string arg_value = params.find<std::string>(arg_name, "" );

			if( "" == arg_value ) {
				if( 0 == arg ) {
					arg_value = elf_info->getBinaryPath();
					output->verbose(CALL_INFO, 8, 0, "--> auto-set \"%s\" to \"%s\"\n",
						arg_name, arg_value.c_str());
				} else {
					output->fatal( CALL_INFO, -1, "Error - unable to find argument %s, value is empty string which is not allowed in Linux.\n", arg_name );
				}
			}

			output->verbose(CALL_INFO, 16, 0, "--> Found %s = \"%s\"\n", arg_name, arg_value.c_str() );

			uint8_t* arg_value_ptr = (uint8_t*) &arg_value.c_str()[0];

			// Record the start
			arg_start_offsets.push_back( (uint64_t) arg_data_block.size() );

			for( size_t j = 0; j < arg_value.size(); ++j ) {
				arg_data_block.push_back( arg_value_ptr[j] );
			}

			arg_data_block.push_back( (uint8_t)('\0') );
		}
		delete[] arg_name;

		char* env_var_name = new char[32];
		std::vector<uint8_t> env_data_block;
		std::vector<uint64_t> env_start_offsets;

		for( uint32_t env_var = 0; env_var < env_count; ++env_var ) {
			snprintf( env_var_name, 32, "env%" PRIu32 "", env_var );
			std::string env_value = params.find<std::string>( env_var_name, "" );

			if( "" == env_value ) {
				output->fatal( CALL_INFO, -1, "Error - unable to find a value for %s, value is empty or non-existent which is not allowed.\n", env_var_name );
			}

			output->verbose(CALL_INFO, 16, 0, "--> Found %s = \"%s\"\n", env_var_name, env_value.c_str() );

			uint8_t* env_value_ptr = (uint8_t*) &env_value.c_str()[0];

			// Record the start
			env_start_offsets.push_back( (uint64_t) env_data_block.size() );

			for( size_t j = 0; j < env_value.size(); ++j ) {
				env_data_block.push_back( env_value_ptr[j] );
			}
			env_data_block.push_back( (uint8_t)('\0') );
		}
//		env_start_offsets.push_back( env_data_block.size() );
//		vanadis_vec_copy_in<char>( env_data_block, '\0' );

		delete[] env_var_name;

		std::vector<uint8_t> phdr_data_block;

		for( int i = 0; i < elf_info->getProgramHeaderEntryCount(); ++i ) {
			const VanadisELFProgramHeaderEntry* nxt_entry = elf_info->getProgramHeader( i );

			vanadis_vec_copy_in<int>( phdr_data_block, (int) nxt_entry->getHeaderTypeNumber() );
			vanadis_vec_copy_in<int>( phdr_data_block, (int) nxt_entry->getImageOffset() );
			vanadis_vec_copy_in<int>( phdr_data_block, (int) nxt_entry->getVirtualMemoryStart() );
			// Physical address - just ignore this for now
			vanadis_vec_copy_in<int>( phdr_data_block, (int) nxt_entry->getPhysicalMemoryStart() );
			vanadis_vec_copy_in<int>( phdr_data_block, (int) nxt_entry->getHeaderImageLength() );
			vanadis_vec_copy_in<int>( phdr_data_block, (int) nxt_entry->getHeaderMemoryLength() );
			vanadis_vec_copy_in<int>( phdr_data_block, (int) nxt_entry->getSegmentFlags() );
			vanadis_vec_copy_in<int>( phdr_data_block, (int) nxt_entry->getAlignment() );
		}

		const uint64_t phdr_address = params.find<uint64_t>("program_header_address", 0x60000000 );

		std::vector<uint8_t> random_values_data_block;

		for(int i = 0; i < 8; ++i ) {
			random_values_data_block.push_back( rand() % 255 );
		}

		const uint64_t rand_values_address = phdr_address + phdr_data_block.size() + 64;

		std::vector<uint8_t> aux_data_block;

		// AT_EXECFD (file descriptor of the executable)
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_EXECFD );
		vanadis_vec_copy_in<int>( aux_data_block, 4    );

		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_PHDR );
		vanadis_vec_copy_in<int>( aux_data_block, (int) phdr_address );

		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_PHENT );
		vanadis_vec_copy_in<int>( aux_data_block, (int) elf_info->getProgramHeaderEntrySize() );

		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_PHNUM );
		vanadis_vec_copy_in<int>( aux_data_block, (int) elf_info->getProgramHeaderEntryCount() );

		// System page size
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_PAGESZ );
		vanadis_vec_copy_in<int>( aux_data_block, 4096 );

		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_ENTRY );
		vanadis_vec_copy_in<int>( aux_data_block, (int) elf_info->getEntryPoint() );

		// AT_BASE (base address loaded into)
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_BASE );
		vanadis_vec_copy_in<int>( aux_data_block, 0    );

		// AT_FLAGS
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_FLAGS );
		vanadis_vec_copy_in<int>( aux_data_block, 0    );

		// AT_HWCAP
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_HWCAP );
		vanadis_vec_copy_in<int>( aux_data_block, 0    );

		// AT_CLKTCK (Clock Tick Resolution)
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_CLKTCK );
		vanadis_vec_copy_in<int>( aux_data_block, 100  );

		// Not ELF
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_NOTELF );
		vanadis_vec_copy_in<int>( aux_data_block, 0    );

		// Real UID
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_UID );
		vanadis_vec_copy_in<int>( aux_data_block, (int) getuid() );

		// Effective UID
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_EUID );
		vanadis_vec_copy_in<int>( aux_data_block, (int) geteuid() );

		// Real GID
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_GID );
		vanadis_vec_copy_in<int>( aux_data_block, (int) getgid() );

		// Effective GID
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_EGID );
		vanadis_vec_copy_in<int>( aux_data_block, (int) getegid() );

		// D-Cache Line Size
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_DCACHEBSIZE );
		vanadis_vec_copy_in<int>( aux_data_block, 64    );

		// I-Cache Line Size
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_ICACHEBSIZE );
		vanadis_vec_copy_in<int>( aux_data_block, 64 	);

		// AT_SECURE?
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_SECURE );
		vanadis_vec_copy_in<int>( aux_data_block, 0                 );

		// AT_RANDOM - 8 bytes of random stuff
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_RANDOM    );
		vanadis_vec_copy_in<int>( aux_data_block, rand_values_address  );

		// End the Auxillary vector
		vanadis_vec_copy_in<int>( aux_data_block, VANADIS_AT_NULL );
		vanadis_vec_copy_in<int>( aux_data_block, 0    );

		// Find out how many AUX entries we added, these should be an int (identifier) and then an int (value) so div by 8
		// but we need to count ints, so really div by 4
		const int aux_entry_count = aux_data_block.size() / 4;

		const int16_t sp_phys_reg = isa_tbl->getIntPhysReg( 29 );

		output->verbose(CALL_INFO, 16, 0, "-> Argument Count:                       %" PRIu32 "\n", arg_count );
		output->verbose(CALL_INFO, 16, 0, "---> Data Size for items:                %" PRIu32 "\n", (uint32_t) arg_data_block.size() );
		output->verbose(CALL_INFO, 16, 0, "-> Environment Variable Count:           %" PRIu32 "\n", (uint32_t) env_start_offsets.size() );
		output->verbose(CALL_INFO, 16, 0, "---> Data size for items:                %" PRIu32 "\n", (uint32_t) env_data_block.size() );
		output->verbose(CALL_INFO, 16, 0, "---> Data size of aux-vector:            %" PRIu32 "\n", (uint32_t) aux_data_block.size() );
		output->verbose(CALL_INFO, 16, 0, "---> Aux entry count:                    %d\n", aux_entry_count);
		//output->verbose(CALL_INFO, 16, 0, "-> Full Startup Data Size:               %" PRIu32 "\n", (uint32_t) stack_data.size() );
		output->verbose(CALL_INFO, 16, 0, "-> Stack Pointer (r29) maps to phys-reg: %" PRIu16 "\n", sp_phys_reg);
		output->verbose(CALL_INFO, 16, 0, "-> Setting SP to (not-aligned):          %" PRIu64 " / 0x%0llx\n",
			start_stack_address, start_stack_address);

		uint64_t arg_env_space_needed = 1 + arg_count + 1 + env_count + 1 + aux_entry_count;
		uint64_t arg_env_space_and_data_needed = (arg_env_space_needed * 4) + arg_data_block.size() + env_data_block.size() + aux_data_block.size();

		uint64_t aligned_start_stack_address = ( start_stack_address - arg_env_space_and_data_needed );
		const uint64_t padding_needed        = ( aligned_start_stack_address % 64 );
		aligned_start_stack_address 	     = aligned_start_stack_address - padding_needed;

		output->verbose(CALL_INFO, 16, 0, "Aligning stack address to 64 bytes (%" PRIu64 " - %" PRIu64 " - padding: %" PRIu64 " = %" PRIu64 " / 0x%0llx)\n",
			start_stack_address, (uint64_t) arg_env_space_and_data_needed, padding_needed, aligned_start_stack_address,
			aligned_start_stack_address );

		start_stack_address = aligned_start_stack_address;

		// Allocate 64 zeros for now
		std::vector<uint8_t> stack_data;

		const uint64_t arg_env_data_start = start_stack_address + (arg_env_space_needed * 4);

		output->verbose(CALL_INFO, 16, 0, "-> Setting start of stack data to:       %" PRIu64 "\n", arg_env_data_start);

		vanadis_vec_copy_in<uint32_t>( stack_data, arg_count );

		for( size_t i = 0; i < arg_start_offsets.size(); ++i ) {
			output->verbose(CALL_INFO, 16, 0, "--> Setting arg%" PRIu32 " to point to address %" PRIu64 " / 0x%llx\n",
				(uint32_t) i, arg_env_data_start + arg_start_offsets[i], arg_env_data_start + arg_start_offsets[i] );
			vanadis_vec_copy_in<uint32_t>( stack_data, (uint32_t)( arg_env_data_start + arg_start_offsets[i] ) );
		}

		vanadis_vec_copy_in<uint32_t>( stack_data, 0 );

		for( size_t i = 0; i < env_start_offsets.size(); ++i ) {
			output->verbose(CALL_INFO, 16, 0, "--> Setting env%" PRIu32 " to point to address %" PRIu64 " / 0x%llx\n",
				(uint32_t) i, arg_env_data_start + arg_data_block.size() + env_start_offsets[i],
					arg_env_data_start + arg_data_block.size() + env_start_offsets[i] );

			vanadis_vec_copy_in<uint32_t>( stack_data, (uint32_t)( arg_env_data_start + arg_data_block.size() + env_start_offsets[i] ) );
		}

		vanadis_vec_copy_in<uint32_t>( stack_data, 0 );

		for( size_t i = 0; i < aux_data_block.size(); ++i ) {
			stack_data.push_back( aux_data_block[i] );
		}

		for( size_t i = 0; i < arg_data_block.size(); ++i ) {
			stack_data.push_back( arg_data_block[i] );
		}

		for( size_t i = 0; i < env_data_block.size(); ++i ) {
			stack_data.push_back( env_data_block[i] );
		}

		for( size_t i = 0; i < padding_needed; ++i ) {
			vanadis_vec_copy_in<uint8_t>( stack_data, (uint8_t) 0 );
		}

		output->verbose(CALL_INFO, 16, 0, "-> Pushing %" PRIu64 " bytes to the start of stack (0x%llx) via memory init event..\n",
			(uint64_t) stack_data.size(), start_stack_address);

		uint64_t index = 0;
		while( index < stack_data.size() ) {
			uint64_t inner_index = 0;

			while(inner_index < 4) {
				//if( index < stack_data.size() ) {
				//	printf("0x%x ", stack_data[index]);
				//}

				index++;
				inner_index++;
			}

			//printf("\n");
		}

		output->verbose(CALL_INFO, 16, 0, "-> Sending inital write of auxillary vector to memory, forms basis of stack start (addr: 0x%llx)\n",
			start_stack_address);

		lsq->setInitialMemory( start_stack_address, stack_data );

		output->verbose(CALL_INFO, 16, 0, "-> Sending initial write of AT_RANDOM values to memory (0x%llx, len: %" PRIu64 ")\n",
			rand_values_address, (uint64_t) random_values_data_block.size());

		lsq->setInitialMemory( rand_values_address, random_values_data_block );

		output->verbose(CALL_INFO, 16, 0, "-> Sending initial data for program headers (addr: 0x%llx, len: %" PRIu64 ")\n", phdr_address,
			(uint64_t) phdr_data_block.size() );

		lsq->setInitialMemory( phdr_address, phdr_data_block );

		output->verbose(CALL_INFO, 16, 0, "-> Setting SP to (64B-aligned):          %" PRIu64 " / 0x%0llx\n",
			start_stack_address, start_stack_address );

		// Set up the stack pointer
		// Register 29 is MIPS for Stack Pointer
		regFile->setIntReg( sp_phys_reg, start_stack_address );
	}

	virtual void tick( SST::Output* output, uint64_t cycle ) {
		output->verbose(CALL_INFO, 16, 0, "-> Decode step for thr: %" PRIu32 "\n", hw_thr);
		output->verbose(CALL_INFO, 16, 0, "---> Max decodes per cycle: %" PRIu16 "\n", max_decodes_per_cycle );

		ins_loader->printStatus( output );

		uint16_t decodes_performed = 0;
		uint16_t uop_bundles_used  = 0;

		for( uint16_t i = 0; i < max_decodes_per_cycle; ++i ) {
			// if the ROB has space, then lets go ahead and
			// decode the input, put it in the queue for issue.
			if( ! thread_rob->full() ) {
				if( ins_loader->hasBundleAt( ip ) ) {
					output->verbose(CALL_INFO, 16, 0, "---> Found uop bundle for ip=0x0%llx, loading from cache...\n", ip);
					VanadisInstructionBundle* bundle = ins_loader->getBundleAt( ip );
					stat_uop_hit->addData(1);

					output->verbose(CALL_INFO, 16, 0, "-----> Bundle contains %" PRIu32 " entries.\n", bundle->getInstructionCount());

					if( 0 == bundle->getInstructionCount() ) {
						output->fatal(CALL_INFO, -1, "------> STOP - bundle at 0x%0llx contains zero entries.\n",
							ip);
					}

					bool q_contains_store = false;
					
					output->verbose(CALL_INFO, 16, 0, "----> thr-rob contains %" PRIu32 " entries.\n", (uint32_t) thread_rob->size());

					// Check if last instruction is a BRANCH, if yes, we need to also decode the branch-delay slot AND handle the prediction
					if( bundle->getInstructionByIndex( bundle->getInstructionCount() - 1 )->getInstFuncType() == INST_BRANCH ) {
						output->verbose(CALL_INFO, 16, 0, "-----> Last instruction in the bundle causes potential branch, checking on branch delay slot\n");

						VanadisInstructionBundle* delay_bundle = nullptr;
						uint32_t temp_delay = 0;

						if( ins_loader->hasBundleAt( ip + 4 ) ) {
							// We have also decoded the branch-delay
							delay_bundle = ins_loader->getBundleAt( ip + 4 );
							stat_uop_hit->addData(1);
						} else {
							output->verbose(CALL_INFO, 16, 0, "-----> Branch delay slot is not currently decoded into a bundle.\n");
							if( ins_loader->hasPredecodeAt( ip + 4 ) ) {
								output->verbose(CALL_INFO, 16, 0, "-----> Branch delay slot is a pre-decode cache item, decode it and keep bundle.\n");
								delay_bundle = new VanadisInstructionBundle( ip + 4 );

								if( ins_loader->getPredecodeBytes( output, ip + 4, (uint8_t*) &temp_delay, sizeof( temp_delay ) ) ) {
									stat_predecode_hit->addData(1);

									decode( output, ip + 4, temp_delay, delay_bundle );
									ins_loader->cacheDecodedBundle( delay_bundle );
									decodes_performed++;
								} else {
									output->fatal(CALL_INFO, -1, "Error: instruction loader has bytes for delay slot at %0llx, but they cannot be retrieved.\n",
										(ip + 4));
								}
							} else {
								output->verbose(CALL_INFO, 16, 0, "-----> Branch delay slot also misses in pre-decode cache, need to request it.\n");
								ins_loader->requestLoadAt( output, ip + 4, 4 );
								stat_ins_bytes_loaded->addData(4);
								stat_predecode_miss->addData(1);
							}
						}

						// We have the branch AND the delay, now lets issue them.
						if( nullptr != delay_bundle ) {
							if( ( bundle->getInstructionCount() + delay_bundle->getInstructionCount() ) <
								( thread_rob->capacity() - thread_rob->size() ) ) {

								output->verbose(CALL_INFO, 16, 0, "---> Proceeding with issue the branch and its delay slot...\n");

								for( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
									VanadisInstruction* next_ins = bundle->getInstructionByIndex( i )->clone();

									output->verbose(CALL_INFO, 16, 0, "---> --> issuing ins addr: 0x0%llx, %s...\n",
										next_ins->getInstructionAddress(),
										next_ins->getInstCode());
										
									thread_rob->push( next_ins );
									
									// if this is the last instruction in the bundle
									// we need to obtain a branch prediction
									if( i == ( bundle->getInstructionCount() - 1 ) ) {
										VanadisSpeculatedInstruction* speculated_ins = 
											dynamic_cast< VanadisSpeculatedInstruction* >( next_ins );
									
										// Do we have an entry for the branch instruction we just issued
										if( branch_predictor->contains(ip) ) {
											const uint64_t predicted_address = branch_predictor->predictAddress( ip );
											speculated_ins->setSpeculatedAddress( predicted_address );

											// This is essential a predicted not taken branch
											if( predicted_address == (ip + 8) ) {
												output->verbose(CALL_INFO, 16, 0, "---> Branch 0x%llx predicted not taken, ip set to: 0x%0llx\n", ip, predicted_address );
//												speculated_ins->setSpeculatedDirection( BRANCH_NOT_TAKEN );
											} else {
												output->verbose(CALL_INFO, 16, 0, "---> Branch 0x%llx predicted taken, jump to 0x%0llx\n", ip, predicted_address);
//												speculated_ins->setSpeculatedDirection( BRANCH_TAKEN );
											}

											ip = predicted_address;
											output->verbose(CALL_INFO, 16, 0, "---> Forcing IP update according to branch prediction table, new-ip: %0llx\n", ip);
										} else {
											output->verbose(CALL_INFO, 16, 0, "---> Branch table does not contain an entry for ins: 0x%0llx, continue with normal ip += 8 = 0x%0llx\n",
												ip, (ip+8) );

//											speculated_ins->setSpeculatedDirection( BRANCH_NOT_TAKEN );
											speculated_ins->setSpeculatedAddress( ip + 8 );

											// We don't urgh.. let's just carry on
											// remember we increment the IP by 2 instructions (me + delay)
											ip += 8;
										}
									}
								}

								for( uint32_t i = 0; i < delay_bundle->getInstructionCount(); ++i ) {
									VanadisInstruction* next_ins = delay_bundle->getInstructionByIndex( i )->clone();

									output->verbose(CALL_INFO, 16, 0, "---> --> issuing ins addr: 0x0%llx, %s...\n",
										next_ins->getInstructionAddress(),
										next_ins->getInstCode());
									thread_rob->push( next_ins);
								}

								uop_bundles_used += 2;
							} else {
								output->verbose(CALL_INFO, 16, 0, "---> --> micro-op for branch and delay exceed decode-q space. Cannot issue this cycle.\n");
								break;
							}
						}
					} else {
						output->verbose(CALL_INFO, 16, 0, "---> Instruction for issue is not a branch, continuing with normal copy to issue-queue...\n");
						// Do we have enough space in the decode queue for the bundle contents?
						if( bundle->getInstructionCount() < (thread_rob->capacity() - thread_rob->size()) ) {
							// Put in the queue
							for( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
								VanadisInstruction* next_ins = bundle->getInstructionByIndex( i );

								output->verbose(CALL_INFO, 16, 0, "---> --> issuing ins addr: 0x0%llx, %s...\n",
									next_ins->getInstructionAddress(),
									next_ins->getInstCode());
								thread_rob->push( next_ins->clone() );
							}

							uop_bundles_used++;

							// Push the instruction pointer along by the standard amount
							ip += 4;
						} else {
							output->verbose(CALL_INFO, 16, 0, "---> --> micro-op bundle for %p contains %" PRIu32 " ops, we only have %" PRIu32 " slots available in the decode q, wait for resources to become available.\n",
								(void*) ip, (uint32_t) bundle->getInstructionCount(),
								(uint32_t) (thread_rob->capacity() - thread_rob->size()));
							// We don't have enough space, so we have to stop and wait for more entries to free up.
							break;
						}
					}
				} else if( ins_loader->hasPredecodeAt( ip ) ) {
					// We do have a locally cached copy of the data at the IP though, so decode into a bundle
					output->verbose(CALL_INFO, 16, 0, "---> uop not found, but matched in predecoded L0-icache (ip=%p)\n", (void*) ip);
					stat_predecode_hit->addData(1);

					uint32_t temp_ins = 0;
					VanadisInstructionBundle* decoded_bundle = new VanadisInstructionBundle( ip );

					if( ins_loader->getPredecodeBytes( output, ip, (uint8_t*) &temp_ins, sizeof(temp_ins) ) ) {
						output->verbose(CALL_INFO, 16, 0, "---> performing a decode of the bytes found (ins-bytes: 0x%x)\n", temp_ins);
						decode( output, ip, temp_ins, decoded_bundle );

						output->verbose(CALL_INFO, 16, 0, "---> performing a decode of the bytes found (generates %" PRIu32 " micro-op bundle).\n",
							(uint32_t) decoded_bundle->getInstructionCount());
						ins_loader->cacheDecodedBundle( decoded_bundle );
						decodes_performed++;

						break;
					} else {
						output->fatal(CALL_INFO, -1, "Error: predecoded bytes found at ip=%p, but %d byte retrival failed.\n",
							(void*) ip, (int) sizeof( temp_ins ));
					}
				} else {
					output->verbose(CALL_INFO, 16, 0, "---> uop bundle and pre-decoded bytes are not found (ip=%p), requesting icache read (line-width=%" PRIu64 ")\n",
						(void*) ip, ins_loader->getCacheLineWidth() );
					ins_loader->requestLoadAt( output, ip, 4 );
					stat_ins_bytes_loaded->addData(4);
					stat_predecode_miss->addData(1);
					break;
				}
			} else {
				output->verbose(CALL_INFO, 16, 0, "---> Decoded pending issue queue is full, no more decodes permitted.\n");
				break;
			}
		}

		output->verbose(CALL_INFO, 16, 0, "---> Performed %" PRIu16 " decodes this cycle, %" PRIu16 " uop-bundles used / updated-ip: 0x%llx.\n",
			decodes_performed, uop_bundles_used, ip);
	}

protected:
	void extract_imm( const uint32_t ins, uint32_t* imm ) const {
		(*imm) = (ins & MIPS_IMM_MASK);
	}

	void extract_three_regs( const uint32_t ins, uint16_t* rt, uint16_t* rs, uint16_t* rd ) const {
		(*rt) = (ins & MIPS_RT_MASK) >> 16;
		(*rs) = (ins & MIPS_RS_MASK) >> 21;
		(*rd) = (ins & MIPS_RD_MASK) >> 11;
	}

	void extract_fp_regs( const uint32_t ins, uint16_t* fr, uint16_t* ft, uint16_t* fs, uint16_t* fd ) const {
		(*fr) = (ins & MIPS_FR_MASK) >> 21;
		(*ft) = (ins & MIPS_FT_MASK) >> 16;
		(*fs) = (ins & MIPS_FS_MASK) >> 11;
		(*fd) = (ins & MIPS_FD_MASK) >> 6;
	}

	void decode( SST::Output* output, const uint64_t ins_addr, const uint32_t next_ins, VanadisInstructionBundle* bundle ) {
		output->verbose( CALL_INFO, 16, 0, "[decode] > addr: 0x%llx ins: 0x%08x\n", ins_addr, next_ins );

		const uint32_t hw_thr    = getHardwareThread();
		const uint32_t ins_mask  = next_ins & MIPS_OP_MASK;
		const uint32_t func_mask = next_ins & MIPS_FUNC_MASK;

		if( 0 != (ins_addr & 0x3) ) {
			output->verbose( CALL_INFO, 16, 0, "[decode] ---> fault address 0x%llu is not aligned at 4 bytes.\n",
				ins_addr);
			bundle->addInstruction( new VanadisInstructionDecodeFault( ins_addr, hw_thr, options ) );
			return;
		}

		if( haltOnDecodeZero && ( 0 == ins_addr ) ) {
			bundle->addInstruction( new VanadisInstructionDecodeFault( ins_addr, getHardwareThread(), options) );
			return;
		}

		output->verbose( CALL_INFO, 16, 0, "[decode] ---> ins-mask: 0x%08x / 0x%08x\n", ins_mask, func_mask );

		uint16_t rt = 0;
		uint16_t rs = 0;
		uint16_t rd = 0;

		uint32_t imm = 0;

		// Perform a register and parameter extract in case we need later
		extract_three_regs( next_ins, &rt, &rs, &rd );
		extract_imm( next_ins, &imm );

		output->verbose( CALL_INFO, 16, 0, "[decode] rt=%" PRIu32 ", rs=%" PRIu32 ", rd=%" PRIu32 "\n",
			rt, rs, rd);

		const uint64_t imm64 = (uint64_t) imm;

		bool insertDecodeFault = true;

		// Check if this is a NOP, this is fairly frequent due to use in delay slots, do not spend time decoding this
		if( 0 == next_ins ) {
			bundle->addInstruction( new VanadisNoOpInstruction( ins_addr, hw_thr, options ) );
			insertDecodeFault = false;
		} else {

		output->verbose(CALL_INFO, 16, 0, "[decode] -> inst-mask: 0x%08x\n", ins_mask);

		switch( ins_mask ) {
		case 0:
			{
				// The SHIFT 5 bits must be zero for these operations according to the manual
				if( 0 == (next_ins & MIPS_SHFT_MASK ) ) {
					output->verbose( CALL_INFO, 16, 0, "[decode] -> special-class, func-mask: 0x%x\n", func_mask);

					if( (0 == func_mask) && (0 == rs) ) {
						output->verbose( CALL_INFO, 16, 0, "[decode] -> rs is also zero, implies truncate (generate: 64 to 32 truncate)\n");
						bundle->addInstruction( new VanadisTruncateInstruction( ins_addr, hw_thr, options, rd, rt, VANADIS_FORMAT_INT64, VANADIS_FORMAT_INT32 ) );
						insertDecodeFault = false;
					} else {
						switch( func_mask ) {
						case MIPS_SPEC_OP_MASK_ADD:
							{
								bundle->addInstruction( new VanadisAddInstruction( ins_addr, hw_thr, options, rd, rs, rt, true, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_ADDU:
							{
								bundle->addInstruction( new VanadisAddInstruction( ins_addr, hw_thr, options, rd, rs, rt, true, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_AND:
							{
								bundle->addInstruction( new VanadisAndInstruction( ins_addr, hw_thr, options, rd, rs, rt ) );
								insertDecodeFault = false;
							}
							break;

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
								bundle->addInstruction( new VanadisDivideRemainderInstruction( ins_addr,
									hw_thr, options, MIPS_REG_LO, MIPS_REG_HI, rs, rt, true, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_DIVU:
							{
								bundle->addInstruction( new VanadisDivideRemainderInstruction( ins_addr,
									hw_thr, options, MIPS_REG_LO, MIPS_REG_HI, rs, rt, false, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

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

								bundle->addInstruction( new VanadisJumpRegInstruction( ins_addr, hw_thr, options, rs,
									VANADIS_SINGLE_DELAY_SLOT ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_JALR:
							{
								bundle->addInstruction( new VanadisJumpRegLinkInstruction( ins_addr, hw_thr, options,
									rd, rs, VANADIS_SINGLE_DELAY_SLOT ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_MFHI:
							{
								// Special instruction_, 32 is LO, 33 is HI
								bundle->addInstruction( new VanadisAddImmInstruction( ins_addr, hw_thr, options, rd,
									MIPS_REG_HI, 0, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_MFLO:
							{
								// Special instruction, 32 is LO, 33 is HI
								bundle->addInstruction( new VanadisAddImmInstruction( ins_addr, hw_thr, options, rd,
									MIPS_REG_LO, 0, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_MOVN:
							break;

						case MIPS_SPEC_OP_MASK_MOVZ:
							break;

						case MIPS_SPEC_OP_MASK_MTHI:
							break;

						case MIPS_SPEC_OP_MASK_MTLO:
							break;

						case MIPS_SPEC_OP_MASK_MULT:
							{
								bundle->addInstruction( new VanadisMultiplySplitInstruction( ins_addr, hw_thr, options,
									MIPS_REG_LO, MIPS_REG_HI, rs, rt, true, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_MULTU:
							{
								bundle->addInstruction( new VanadisMultiplySplitInstruction( ins_addr, hw_thr, options,
									MIPS_REG_LO, MIPS_REG_HI, rs, rt, false, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_NOR:
							{
								bundle->addInstruction( new VanadisNorInstruction( ins_addr, hw_thr, options, rd, rs, rt ) );
															insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_OR:
							{
								bundle->addInstruction( new VanadisOrInstruction( ins_addr, hw_thr, options, rd, rs, rt ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_SLLV:
							{
								bundle->addInstruction( new VanadisShiftLeftLogicalInstruction( ins_addr,
									hw_thr, options, rd, rt, rs, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_SLT:
							{
								bundle->addInstruction( new VanadisSetRegCompareInstruction( ins_addr, hw_thr, options,
									rd, rs, rt, true, REG_COMPARE_LT, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_SLTU:
							{
								bundle->addInstruction( new VanadisSetRegCompareInstruction( ins_addr, hw_thr, options,
									rd, rs, rt, false, REG_COMPARE_LT, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_SRAV:
							{
								bundle->addInstruction( new VanadisShiftRightArithmeticInstruction( ins_addr, hw_thr, options,
									rd, rt, rs, VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_SRLV:
							{
								bundle->addInstruction( new VanadisShiftRightLogicalInstruction( ins_addr, hw_thr, options,
									rd, rt, rs, VANADIS_FORMAT_INT32) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_SUB:
							{
								bundle->addInstruction( new VanadisSubInstruction( ins_addr, hw_thr, options, rd, rs, rt, true,
									VANADIS_FORMAT_INT32  ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_SUBU:
							{
								bundle->addInstruction( new VanadisSubInstruction( ins_addr, hw_thr, options, rd, rs, rt, false,
									VANADIS_FORMAT_INT32 ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_SYSCALL:
							{
								bundle->addInstruction( new VanadisSysCallInstruction( ins_addr, hw_thr, options ) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_SYNC:
							{
								bundle->addInstruction( new VanadisFenceInstruction( ins_addr, hw_thr, options,
									VANADIS_LOAD_STORE_FENCE) );
								insertDecodeFault = false;
							}
							break;

						case MIPS_SPEC_OP_MASK_XOR:
							bundle->addInstruction( new VanadisXorInstruction( ins_addr, hw_thr, options, rd, rs, rt ) );
							insertDecodeFault = false;
							break;
						}
					}
				} else {
					switch( func_mask ) {
					case MIPS_SPEC_OP_MASK_SLL:
						{
							const uint64_t shf_amnt = ((uint64_t) (next_ins & MIPS_SHFT_MASK)) >> 6;

							output->verbose(CALL_INFO, 16, 0, "[decode/SLL]-> out: %" PRIu16 " / in: %" PRIu16 " shft: %" PRIu64 "\n",
								rd, rt, shf_amnt);

							bundle->addInstruction( new VanadisShiftLeftLogicalImmInstruction( ins_addr,
								hw_thr, options, rd, rt, shf_amnt, VANADIS_FORMAT_INT32 ) );
							insertDecodeFault = false;
						}
						break;

					case MIPS_SPEC_OP_MASK_SRL:
						{
							const uint64_t shf_amnt = ((uint64_t) (next_ins & MIPS_SHFT_MASK)) >> 6;

							output->verbose(CALL_INFO, 16, 0, "[decode/SRL]-> out: %" PRIu16 " / in: %" PRIu16 " shft: %" PRIu64 "\n",
								rd, rt, shf_amnt);

							bundle->addInstruction( new VanadisShiftRightLogicalImmInstruction( ins_addr, hw_thr, options,
								rd, rt, shf_amnt, VANADIS_FORMAT_INT32 ) );
							insertDecodeFault = false;
						}
						break;

					case MIPS_SPEC_OP_MASK_SRA:
						{
							const uint64_t shf_amnt = ((uint64_t) (next_ins & MIPS_SHFT_MASK)) >> 6;

							bundle->addInstruction( new VanadisShiftRightArithmeticImmInstruction( ins_addr, hw_thr, options,
								rd, rt, shf_amnt, VANADIS_FORMAT_INT32 ) );
							insertDecodeFault = false;
						}
						break;
					}
				}
			}
			break;

		case MIPS_SPEC_OP_MASK_REGIMM:
			{
				const uint64_t offset_value_64 = vanadis_sign_extend_offset_16_and_shift( next_ins, 2 );;

#ifdef VANADIS_BUILD_DEBUG
				output->verbose(CALL_INFO, 16, 0, "[decoder/REGIMM] -> imm: %" PRIu64 "\n", offset_value_64);
				output->verbose(CALL_INFO, 16, 0, "[decoder]        -> rt: 0x%08x\n", (next_ins & MIPS_RT_MASK));
#endif

				switch( ( next_ins & MIPS_RT_MASK ) ) {
				case MIPS_SPEC_OP_MASK_BLTZ:
					{
						bundle->addInstruction( new VanadisBranchRegCompareImmInstruction(ins_addr, hw_thr, options,
							rs, 0, offset_value_64, VANADIS_SINGLE_DELAY_SLOT, REG_COMPARE_LT, VANADIS_FORMAT_INT32 ) );
						insertDecodeFault = false;
					}
					break;
				case MIPS_SPEC_OP_MASK_BGEZAL:
					{
						bundle->addInstruction( new VanadisBranchRegCompareImmLinkInstruction( ins_addr, hw_thr, options,
							rs, 0, offset_value_64, (uint16_t) 31, VANADIS_SINGLE_DELAY_SLOT, REG_COMPARE_GTE, VANADIS_FORMAT_INT32 ) );
						insertDecodeFault = false;
					}
					break;
				case MIPS_SPEC_OP_MASK_BGEZ:
					{
						bundle->addInstruction( new VanadisBranchRegCompareImmInstruction(ins_addr, hw_thr, options,
							rs, 0, offset_value_64, VANADIS_SINGLE_DELAY_SLOT, REG_COMPARE_GTE, VANADIS_FORMAT_INT32 ) );
						insertDecodeFault = false;
					}
					break;
				}

			}
			break;

		case MIPS_SPEC_OP_MASK_LUI:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift( next_ins, 16 );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/LUI] -> reg: %" PRIu16 " / imm=%" PRId64 "\n",
//					rt, imm_value_64);
				bundle->addInstruction( new VanadisSetRegisterInstruction( ins_addr, hw_thr, options, rt, imm_value_64, VANADIS_FORMAT_INT32 ) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_LB:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//                                output->verbose(CALL_INFO, 16, 0, "[decoder/LB]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n",
//                                        rt, rs, imm_value_64);

                                bundle->addInstruction( new VanadisLoadInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
                                        rt, 1, true, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER ) );
                                insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_LBU:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//                                output->verbose(CALL_INFO, 16, 0, "[decoder/LBU]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n",
//                                        rt, rs, imm_value_64);

                                bundle->addInstruction( new VanadisLoadInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
                                        rt, 1, false, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER) );
                                insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_LW:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/LW]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisLoadInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 4, true, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_LFP32:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/LFP32]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisLoadInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 4, true, MEM_TRANSACTION_NONE, LOAD_FP_REGISTER) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_LL:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/LL]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisLoadInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 4, true, MEM_TRANSACTION_LLSC_LOAD, LOAD_INT_REGISTER) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_LWL:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/LWL (PARTLOAD)]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n",
//                                        rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisPartialLoadInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 4, true, false, LOAD_INT_REGISTER ) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_LWR:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/LWR (PARTLOAD)]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n",
//                                        rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisPartialLoadInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 4, true, true, LOAD_INT_REGISTER ) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_LHU:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/LHU]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisLoadInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 2, false, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_SB:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/SB]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisStoreInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 1, MEM_TRANSACTION_NONE, STORE_INT_REGISTER) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_SC:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/SC]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisStoreInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 4, MEM_TRANSACTION_LLSC_STORE, STORE_INT_REGISTER) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_SW:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/SW]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisStoreInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 4, MEM_TRANSACTION_NONE, STORE_INT_REGISTER) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_SH:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/SH]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisStoreInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 2, MEM_TRANSACTION_NONE, STORE_INT_REGISTER) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_SFP32:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/SFP32]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisStoreInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 4, MEM_TRANSACTION_NONE, STORE_FP_REGISTER) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_SWL:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/SWL]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisPartialStoreInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 4, true, STORE_INT_REGISTER ) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_SWR:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/SWR]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisPartialStoreInstruction( ins_addr, hw_thr, options, rs, imm_value_64,
					rt, 4, false, STORE_INT_REGISTER ) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_ADDIU:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );
//				output->verbose(CALL_INFO, 16, 0, "[decoder/ADDIU]: -> reg: %" PRIu16 " rs=%" PRIu16 " / imm=%" PRId64 "\n",
//					rt, rs, imm_value_64);
				bundle->addInstruction( new VanadisAddImmInstruction( ins_addr, hw_thr, options, rt, rs, imm_value_64, VANADIS_FORMAT_INT32 ) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_BEQ:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift( next_ins, 2 );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/BEQ]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64 "\n",
//                                        rt, rs, imm_value_64 );
				bundle->addInstruction( new VanadisBranchRegCompareInstruction( ins_addr, hw_thr, options, rt, rs,
					imm_value_64, VANADIS_SINGLE_DELAY_SLOT, REG_COMPARE_EQ, VANADIS_FORMAT_INT32) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_BGTZ:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift( next_ins, 2 );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/BGTZ]: -> r1: %" PRIu16 " offset: %" PRId64 "\n",
//                                        rs, imm_value_64);
				bundle->addInstruction( new VanadisBranchRegCompareImmInstruction( ins_addr, hw_thr, options, rs, 0,
					imm_value_64, VANADIS_SINGLE_DELAY_SLOT, REG_COMPARE_GT, VANADIS_FORMAT_INT32) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_BLEZ:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift( next_ins, 2 );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/BLEZ]: -> r1: %" PRIu16 " offset: %" PRId64 "\n",
//                                        rs, imm_value_64);
				bundle->addInstruction( new VanadisBranchRegCompareImmInstruction( ins_addr, hw_thr, options, rs, 0,
					imm_value_64, VANADIS_SINGLE_DELAY_SLOT, REG_COMPARE_LTE, VANADIS_FORMAT_INT32) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_BNE:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift( next_ins, 2 );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/BNE]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64 "\n",
//                                        rt, rs, imm_value_64 );
				bundle->addInstruction( new VanadisBranchRegCompareInstruction( ins_addr, hw_thr, options, rt, rs,
					imm_value_64, VANADIS_SINGLE_DELAY_SLOT, REG_COMPARE_NEQ, VANADIS_FORMAT_INT32) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_SLTI:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/SLTI]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64 "\n",
//                                        rt, rs, imm_value_64 );
				bundle->addInstruction( new VanadisSetRegCompareImmInstruction( ins_addr, hw_thr, options,
					rt, rs, imm_value_64, true, REG_COMPARE_LT, VANADIS_FORMAT_INT32 ) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_SLTIU:
			{
				const int64_t imm_value_64 = vanadis_sign_extend_offset_16( next_ins );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/SLTIU]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64 "\n",
//                                        rt, rs, imm_value_64 );
				bundle->addInstruction( new VanadisSetRegCompareImmInstruction( ins_addr, hw_thr, options,
					rt, rs, imm_value_64, false, REG_COMPARE_LT, VANADIS_FORMAT_INT32 ) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_ANDI:
			{
				// note - ANDI is zero extended, not sign extended
				const uint64_t imm_value_64 = static_cast<uint64_t>( next_ins & MIPS_IMM_MASK );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/ANDI]: -> %" PRIu16 " <- r2: %" PRIu16 " imm: %" PRIu64 "\n",
//                                        rt, rs, imm_value_64 );
				bundle->addInstruction( new VanadisAndImmInstruction( ins_addr, hw_thr, options,
					rt, rs, imm_value_64) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_ORI:
			{
				const uint64_t imm_value_64 = static_cast<uint64_t>( next_ins & MIPS_IMM_MASK );

//				output->verbose(CALL_INFO, 16, 0, "[decoder/ORI]: -> %" PRIu16 " <- r2: %" PRIu16 " imm: %" PRId64 "\n",
//                                        rt, rs, imm_value_64 );
				bundle->addInstruction( new VanadisOrImmInstruction( ins_addr, hw_thr, options,
					rt, rs, imm_value_64) );
				insertDecodeFault = false;
			}
			break;

		case MIPS_SPEC_OP_MASK_J:
			{
				const uint32_t j_addr_index = (next_ins & MIPS_J_ADDR_MASK) << 2;
				const uint32_t upper_bits   = ((ins_addr + 4) & MIPS_J_UPPER_MASK);

				uint64_t jump_to = 0;
				jump_to += (uint64_t) j_addr_index;
				jump_to |= (uint64_t) upper_bits;

//				output->verbose(CALL_INFO, 16, 0, "[decoder/J]: -> jump-to: %" PRIu64 " / 0x%0llx\n",
//					jump_to, jump_to);

                                bundle->addInstruction( new VanadisJumpInstruction( ins_addr, hw_thr, options, jump_to, VANADIS_SINGLE_DELAY_SLOT ) );
				insertDecodeFault = false;
			}
			break;
		case MIPS_SPEC_OP_MASK_JAL:
			{
				const uint32_t j_addr_index = (next_ins & MIPS_J_ADDR_MASK) << 2;
                                const uint32_t upper_bits   = ((ins_addr + 4) & MIPS_J_UPPER_MASK);

				uint64_t jump_to = 0;
                                jump_to = jump_to + (uint64_t) j_addr_index;
                                jump_to = jump_to + (uint64_t) upper_bits;

//				bundle->addInstruction( new VanadisSetRegisterInstruction( ins_addr, hw_thr, options, 31, ins_addr + 8 ) );
				bundle->addInstruction( new VanadisJumpLinkInstruction( ins_addr, hw_thr, options, 31, jump_to,
					VANADIS_SINGLE_DELAY_SLOT ) );
				insertDecodeFault = false;
			}
			break;


		case MIPS_SPEC_OP_MASK_XORI:
			{
				const uint64_t xor_mask = static_cast<uint64_t>( next_ins & MIPS_IMM_MASK );

				bundle->addInstruction( new VanadisXorImmInstruction( ins_addr,
					hw_thr, options, rt, rs, xor_mask ) );
				insertDecodeFault = false;
			}

		case MIPS_SPEC_OP_SPECIAL3:
			{
//				output->verbose(CALL_INFO, 16, 0, "[decoder, partial: special3], further decode required...\n");

				switch( next_ins & 0x3F ) {
				case MIPS_SPEC_OP_MASK_RDHWR:
					{
						const uint16_t target_reg = rt;
						const uint16_t req_type   = rd;

//						output->verbose(CALL_INFO, 16, 0, "[decode/RDHWR] target: %" PRIu16 " type: %" PRIu16 "\n",
//							target_reg, req_type);

						switch( rd ) {
						case 29:
							bundle->addInstruction( new VanadisSetRegisterInstruction( ins_addr, hw_thr, options,
								target_reg, (int64_t) getThreadLocalStoragePointer(), VANADIS_FORMAT_INT32 ) );
							insertDecodeFault = false;
							break;
						}
					}
					break;
				}
			}
			break;

		case MIPS_SPEC_OP_MASK_COP1:
			{
//				output->verbose(CALL_INFO, 16, 0, "[decode] --> reached co-processor function decoder\n");

				uint16_t fr = 0;
				uint16_t ft = 0;
				uint16_t fs = 0;
				uint16_t fd = 0;

				extract_fp_regs( next_ins, &fr, &ft, &fs, &fd );

				if( ( next_ins & 0x3E30000 ) == 0x1010000 ) {
					// this decodes to a BRANCH on TRUE
					const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift( next_ins, 2 );

					bundle->addInstruction( new VanadisBranchFPInstruction(
						ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, imm_value_64,
						/* branch on true */ true, VANADIS_SINGLE_DELAY_SLOT ) );
					insertDecodeFault = false;
				} else if( ( next_ins & 0x3E30000 ) == 0x1000000 ) {
					// this decodes to a BRANCH on FALSE
					const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift( next_ins, 2 );

					bundle->addInstruction( new VanadisBranchFPInstruction(
						ins_addr, hw_thr, options, MIPS_FP_STATUS_REG, imm_value_64,
						/* branch on false */ false, VANADIS_SINGLE_DELAY_SLOT ) );
					insertDecodeFault = false;
				} else {

//				output->verbose(CALL_INFO, 16, 0, "[decoder] ----> decoding function mask: %" PRIu32 " / 0x%x\n",
//					(next_ins & MIPS_FUNC_MASK), (next_ins & MIPS_FUNC_MASK) );

				switch( next_ins & MIPS_FUNC_MASK ) {
				case 0:
					{
						if( ( 0 == fd ) && ( MIPS_SPEC_COP_MASK_MTC == fr ) ) {
							bundle->addInstruction( new VanadisGPR2FPInstruction(
								ins_addr, hw_thr, options,
								fs, rt, VANADIS_FORMAT_FP32 ) );
							insertDecodeFault = false;
						} else if( ( 0 == fd ) && ( MIPS_SPEC_COP_MASK_MFC == fr ) ) {
							bundle->addInstruction( new VanadisFP2GPRInstruction(
								ins_addr, hw_thr, options,
								rt, fs, VANADIS_FORMAT_FP32 ) );
							insertDecodeFault = false;
						} else if( ( 0 == fd ) && ( MIPS_SPEC_COP_MASK_CF == fr ) ) {
							uint16_t fp_ctrl_reg = 0;
                                                        bool fp_matched = false;

                                                        switch( rd ) {
                                                        case 0:  fp_ctrl_reg = MIPS_FP_VER_REG; fp_matched = true; break;
                                                        case 31: fp_ctrl_reg = MIPS_FP_STATUS_REG; fp_matched = true; break;
                                                        default:
                                                                break;
                                                        }

                                                        if( fp_matched ) {
								bundle->addInstruction( new VanadisFP2GPRInstruction(
									ins_addr, hw_thr, options,
									rt, fp_ctrl_reg, VANADIS_FORMAT_FP32 ) );
								insertDecodeFault = false;
							}
						} else if( ( 0 == fd ) && ( MIPS_SPEC_COP_MASK_CT == fr ) ) {
							uint16_t fp_ctrl_reg = 0;
							bool fp_matched = false;

							switch( rd ) {
							case 0:  fp_ctrl_reg = MIPS_FP_VER_REG; fp_matched = true; break;
							case 31: fp_ctrl_reg = MIPS_FP_STATUS_REG; fp_matched = true; break;
							default:
								break;
							}

							if( fp_matched ) {
								bundle->addInstruction( new VanadisGPR2FPInstruction(
									ins_addr, hw_thr, options,
									fp_ctrl_reg, rt, VANADIS_FORMAT_FP32 ) );
								insertDecodeFault = false;
							}
						} else {
							// assume this is an FADD and begin decode
							VanadisRegisterFormat input_format = VANADIS_FORMAT_FP64;
       		                                        bool format_fault = false;

                	                                switch( fr ) {
                        	                        case 16: input_format = VANADIS_FORMAT_FP32;  break;
                                	                case 17: input_format = VANADIS_FORMAT_FP64;  break;
                                       	         	case 20: input_format = VANADIS_FORMAT_INT32; break;
                                                	case 21: input_format = VANADIS_FORMAT_INT64; break;
                                                	default:
                                                        	format_fault = true;
                                                        	break;
                                                	}

                                                	if( ! format_fault ) {
								bundle->addInstruction( new VanadisFPAddInstruction(
									ins_addr, hw_thr, options,
									fd, fs, ft, input_format) );
								insertDecodeFault = false;
							}
						}
					}
					break;

				case MIPS_SPEC_COP_MASK_MOV:
					{
						// Decide operand format
						switch( fr ) {
						case 16:
							{
								bundle->addInstruction( new VanadisFP2FPInstruction(
									ins_addr, hw_thr, options,
									fd, fs, VANADIS_FORMAT_FP32 ) );
								insertDecodeFault = false;
							}
							break;
						case 17:
							{
								bundle->addInstruction( new VanadisFP2FPInstruction(
									ins_addr, hw_thr, options,
									fd, fs, VANADIS_FORMAT_FP64 ) );
								insertDecodeFault = false;
							}
							break;
						}
					}
					break;

				case MIPS_SPEC_COP_MASK_MUL:
					{
						VanadisRegisterFormat input_format = VANADIS_FORMAT_FP64;
                                                bool format_fault = false;

                                                switch( fr ) {
                                                case 16: input_format = VANADIS_FORMAT_FP32;  break;
                                                case 17: input_format = VANADIS_FORMAT_FP64;  break;
                                                case 20: input_format = VANADIS_FORMAT_INT32; break;
                                                case 21: input_format = VANADIS_FORMAT_INT64; break;
                                                default:
                                                        format_fault = true;
                                                        break;
                                                }

                                                if( ! format_fault ) {
							bundle->addInstruction( new VanadisFPMultiplyInstruction(
								ins_addr, hw_thr, options,
								fd, fs, ft, input_format) );
							insertDecodeFault = false;
						} else {
//							output->verbose(CALL_INFO, 16, 0, "[decoder] ---> convert function failed because of input-format error (fmt: %" PRIu16 ")\n", fr);
						}
					}
					break;

				case MIPS_SPEC_COP_MASK_DIV:
					{
						VanadisRegisterFormat input_format = VANADIS_FORMAT_FP64;
                                                bool format_fault = false;

                                                switch( fr ) {
                                                case 16: input_format = VANADIS_FORMAT_FP32;  break;
                                                case 17: input_format = VANADIS_FORMAT_FP64;  break;
                                                case 20: input_format = VANADIS_FORMAT_INT32; break;
                                                case 21: input_format = VANADIS_FORMAT_INT64; break;
                                                default:
                                                        format_fault = true;
                                                        break;
                                                }

                                                if( ! format_fault ) {
							bundle->addInstruction( new VanadisFPDivideInstruction(
								ins_addr, hw_thr, options,
								fd, fs, ft, input_format) );
							insertDecodeFault = false;
						} else {
//							output->verbose(CALL_INFO, 16, 0, "[decoder] ---> convert function failed because of input-format error (fmt: %" PRIu16 ")\n", fr);
						}
					}
					break;

				case MIPS_SPEC_COP_MASK_SUB:
					{
						VanadisRegisterFormat input_format = VANADIS_FORMAT_FP64;
                                                bool format_fault = false;

                                                switch( fr ) {
                                                case 16: input_format = VANADIS_FORMAT_FP32;  break;
                                                case 17: input_format = VANADIS_FORMAT_FP64;  break;
                                                case 20: input_format = VANADIS_FORMAT_INT32; break;
                                                case 21: input_format = VANADIS_FORMAT_INT64; break;
                                                default:
                                                        format_fault = true;
                                                        break;
                                                }

                                                if( ! format_fault ) {
							bundle->addInstruction( new VanadisFPSubInstruction(
								ins_addr, hw_thr, options,
								fd, fs, ft, input_format) );
							insertDecodeFault = false;
						} else {
//							output->verbose(CALL_INFO, 16, 0, "[decoder] ---> convert function failed because of input-format error (fmt: %" PRIu16 ")\n", fr);
						}
					}
					break;

				case MIPS_SPEC_COP_MASK_CVTS:
					{
						VanadisRegisterFormat input_format = VANADIS_FORMAT_FP32;
						bool format_fault = false;

						switch( fr ) {
						case 16: input_format = VANADIS_FORMAT_FP32;  break;
						case 17: input_format = VANADIS_FORMAT_FP64;  break;
						case 20: input_format = VANADIS_FORMAT_INT32; break;
						case 21: input_format = VANADIS_FORMAT_INT64; break;
						default:
							format_fault = true;
							break;
						}

						if( ! format_fault ) {
							bundle->addInstruction( new VanadisFPConvertInstruction(
								ins_addr, hw_thr, options,
								fd, fs, input_format, VANADIS_FORMAT_FP32 ) );
							insertDecodeFault = false;
						} else {
//							output->verbose(CALL_INFO, 16, 0, "[decoder] ---> convert function failed because of input-format error (fmt: %" PRIu16 ")\n", fr);
						}
					}

					break;

				case MIPS_SPEC_COP_MASK_CVTD:
					{
						VanadisRegisterFormat input_format = VANADIS_FORMAT_FP64;
						bool format_fault = false;

						switch( fr ) {
						case 16: input_format = VANADIS_FORMAT_FP32;  break;
						case 17: input_format = VANADIS_FORMAT_FP64;  break;
						case 20: input_format = VANADIS_FORMAT_INT32; break;
						case 21: input_format = VANADIS_FORMAT_INT64; break;
						default:
							format_fault = true;
							break;
						}

						if( ! format_fault ) {
							bundle->addInstruction( new VanadisFPConvertInstruction(
								ins_addr, hw_thr, options,
								fd, fs, input_format, VANADIS_FORMAT_FP64 ) );
							insertDecodeFault = false;
						} else {
//							output->verbose(CALL_INFO, 16, 0, "[decoder] ---> convert function failed because of input-format error (fmt: %" PRIu16 ")\n", fr);
						}
					}

					break;

				case MIPS_SPEC_COP_MASK_CVTW:
					{
						VanadisRegisterFormat input_format = VANADIS_FORMAT_FP64;
						bool format_fault = false;

						switch( fr ) {
						case 16: input_format = VANADIS_FORMAT_FP32;  break;
						case 17: input_format = VANADIS_FORMAT_FP64;  break;
						case 20: input_format = VANADIS_FORMAT_INT32; break;
						case 21: input_format = VANADIS_FORMAT_INT64; break;
						default:
							format_fault = true;
							break;
						}

						if( ! format_fault ) {
							bundle->addInstruction( new VanadisFPConvertInstruction(
								ins_addr, hw_thr, options,
								fd, fs, input_format, VANADIS_FORMAT_INT32 ) );
							insertDecodeFault = false;
						} else {
//							output->verbose(CALL_INFO, 16, 0, "[decoder] ---> convert function failed because of input-format error (fmt: %" PRIu16 ")\n", fr);
						}
					}

					break;

				case MIPS_SPEC_COP_MASK_CMP_LT:
				case MIPS_SPEC_COP_MASK_CMP_LTE:
				case MIPS_SPEC_COP_MASK_CMP_EQ:
					{
						VanadisRegisterFormat input_format = VANADIS_FORMAT_FP64;
						bool format_fault = false;

						switch( fr ) {
						case 16: input_format = VANADIS_FORMAT_FP32;  break;
						case 17: input_format = VANADIS_FORMAT_FP64;  break;
						case 20: input_format = VANADIS_FORMAT_INT32; break;
						case 21: input_format = VANADIS_FORMAT_INT64; break;
						default:
							format_fault = true;
							break;
						}

						VanadisRegisterCompareType compare_type = REG_COMPARE_EQ;
						bool compare_fault = false;

						switch( next_ins & 0xF ) {
						case 0x2:  compare_type = REG_COMPARE_EQ;  break;
						case 0xC:  compare_type = REG_COMPARE_LT;  break;
						case 0xE:  compare_type = REG_COMPARE_LTE; break;
						default:
							compare_fault = true;
							break;
						}

						// if neither are true, then we have a good decode, otherwise a problem.
						// register 31 is where condition codes and rounding modes are kept
						if( ! (format_fault | compare_fault) ) {
							bundle->addInstruction( new VanadisFPSetRegCompareInstruction(
								ins_addr, hw_thr, options,
								MIPS_FP_STATUS_REG, fs, ft, input_format, compare_type ) );
							insertDecodeFault = false;
						}
					}
					break;
				}
				}
			}
			break;
		default:
			break;
		}
		}

		if( insertDecodeFault ) {
			bundle->addInstruction( new VanadisInstructionDecodeFault( ins_addr, getHardwareThread(), options) );
			stat_decode_fault->addData(1);
		} else {
			stat_uop_generated->addData( bundle->getInstructionCount() );
		}

		for( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
			output->verbose(CALL_INFO, 16, 0, "-> [%3" PRIu32 "]: %s\n", i, bundle->getInstructionByIndex(i)->getInstCode());
		}

		// Mark the end of a micro-op group so we can count real instructions and not just micro-ops
		if( bundle->getInstructionCount() > 0 ) {
			bundle->getInstructionByIndex( bundle->getInstructionCount() - 1 )->markEndOfMicroOpGroup();
		}

	}

	const VanadisDecoderOptions* options;

	uint64_t start_stack_address;

	bool haltOnDecodeZero;

	uint16_t icache_max_bytes_per_cycle;
	uint16_t max_decodes_per_cycle;
	uint16_t decode_buffer_max_entries;
};

}
}

#endif

