
#ifndef _H_VANADIS_ELF_INFO
#define _H_VANADIS_ELF_INFO

#include <cinttypes>
#include <cstdio>

namespace SST {
namespace Vanadis {

enum VanadisELFISA {
        ISA_X86,
        ISA_MIPS,
        ISA_AMD64,
        ISA_RISC_V,
        ISA_UNKNOWN
};

enum VanadisELFOSABI {
        OS_ABI_LINUX,
	OS_ABI_SYS_V,
        OS_ABI_UNKNOWN
};

enum VanadisELFObjectType {
        OBJ_TYPE_NONE,
        OBJ_TYPE_REL,
        OBJ_TYPE_EXEC,
        OBJ_TYPE_DYN,
        OBJ_TYPE_UNKNOWN
};

const char* getELFObjectTypeString( VanadisELFObjectType obj ) {
	switch( obj ) {
	case OBJ_TYPE_NONE:
		return "NONE";
	case OBJ_TYPE_REL:
		return "REL";
	case OBJ_TYPE_EXEC:
		return "Executable";
	case OBJ_TYPE_DYN:
		return "DYN";
	case OBJ_TYPE_UNKNOWN:
	default:
		return "Unknown";		
	}
}

const char* getELFISAString( VanadisELFISA isa ) {
	switch( isa ) {
	case ISA_X86:
		return "X86";
	case ISA_MIPS:
		return "MIPS";
	case ISA_AMD64:
		return "AMD64";
	case ISA_RISC_V:
		return "RISC-V";
	default:
		return "Unknown";
	}
}

const char* getELFOSABIString( VanadisELFOSABI abi ) {
	switch( abi ) {
	case OS_ABI_LINUX:
		return "Linux";
	case OS_ABI_SYS_V:
		return "System-V";
	default:
		return "Unknown";
	}
}

class VanadisELFInfo {
public:
	VanadisELFInfo() {
		bin_path = nullptr;
		elf_class = UINT8_MAX;
		elf_endian = UINT8_MAX;
		elf_os_abi = UINT8_MAX;
		elf_abi_version = UINT8_MAX;
		elf_obj_type = UINT16_MAX;
		elf_isa = UINT16_MAX;
		elf_entry_point = UINT64_MAX;
		elf_prog_header_start = UINT64_MAX;
		elf_prog_section_start = UINT64_MAX;
		elf_prog_header_entry_size = UINT16_MAX;
		elf_prog_header_entry_count = UINT16_MAX;
		elf_prog_section_entry_size = UINT16_MAX;
		elf_prog_section_entry_count = UINT16_MAX;
		elf_section_header_name_location = UINT16_MAX;
	}

	void print( SST::Output* output ) {
		output->verbose(CALL_INFO, 2, 0, "-------------------------------------------------------\n");
		output->verbose(CALL_INFO, 2, 0, "ELF Information [%s]\n", (nullptr == bin_path) ?
			"Not set" : bin_path );
		output->verbose(CALL_INFO, 2, 0, "-------------------------------------------------------\n");
		output->verbose(CALL_INFO, 2, 0, "Class:                %s\n",
			isELF64() ? "64-bit" : isELF32() ? "32-bit" : "Unknown");
		output->verbose(CALL_INFO, 2, 0, "Instruction-Set:      %" PRIu16 " / %s\n",
			elf_isa, getELFISAString( getISA() ) );
		output->verbose(CALL_INFO, 2, 0, "Object Type:          %s\n",
			getELFObjectTypeString( getObjectType() ) );
		output->verbose(CALL_INFO, 2, 0, "OS-ABI:               %" PRIu8 " / %s\n",
			elf_os_abi, getELFOSABIString( getOSABI() ) );
		output->verbose(CALL_INFO, 2, 0, "Entry-Point:          %p\n",
			(void*) getEntryPoint() );
		output->verbose(CALL_INFO, 2, 0, "Prog-Hdr Offset:      %p\n",
			(void*) getProgramHeaderOffset() );
		output->verbose(CALL_INFO, 2, 0, "Prog-Hdr Entry Size:  %" PRIu16 "\n",
			getProgramHeaderEntrySize() );
		output->verbose(CALL_INFO, 2, 0, "Prog-Hdr Entry Count: %" PRIu16 "\n",
			getProgramHeaderEntryCount() );
		output->verbose(CALL_INFO, 2, 0, "Sect-Hdr Entry Size:  %" PRIu16 "\n",
			getSectionHeaderEntrySize() );
		output->verbose(CALL_INFO, 2, 0, "Sect-Hdr Entry Count: %" PRIu16 "\n",
			getSectionHeaderEntryCount() );
		output->verbose(CALL_INFO, 2, 0, "Sect-Hdr Strings Loc: %" PRIu16 "\n",
			getSectionHeaderNameEntryOffset() );
		output->verbose(CALL_INFO, 2, 0, "-------------------------------------------------------\n");
	}

	void setClass( const uint8_t new_class )    { elf_class  = new_class;  }
	void setEndian( const uint8_t new_endian )  { elf_endian = new_endian; }	
	void setOSABI( const uint8_t new_os_abi )   { elf_os_abi = new_os_abi; }
	void setOSABIVersion( const uint8_t new_abi_ver ) { elf_abi_version = new_abi_ver; }
	void setObjectType( const uint16_t new_obj_type ) { elf_obj_type = new_obj_type; }
	void setISA( const uint16_t new_isa )       { elf_isa = new_isa; }
	void setEntryPoint( const uint64_t new_entry ) { elf_entry_point = new_entry; }
	void setProgramHeaderOffset( const uint64_t new_off ) { elf_prog_header_start = new_off; }
	void setSectionHeaderOffset( const uint64_t new_off ) { elf_prog_section_start = new_off; }
	void setProgramHeaderEntrySize( const uint16_t new_entry_size ) { 
		elf_prog_header_entry_size = new_entry_size; }
	void setProgramHeaderEntryCount( const uint16_t new_entry_count ) {
		elf_prog_header_entry_count = new_entry_count; }
	void setSectionHeaderEntrySize( const uint16_t new_entry_size ) {
		elf_prog_section_entry_size = new_entry_size; }
	void setSectionHeaderEntryCount( const uint16_t new_entry_count ) {
		elf_prog_section_entry_count = new_entry_count; }
	void setSectionEntryIndexForNames( const uint16_t new_entry_index ) {
		elf_section_header_name_location = new_entry_index; }

	bool isELF64() const {
		return (2 == elf_class);
	}

	bool isELF32() const {
		return (1 == elf_class);
	}

	VanadisELFISA getISA() const {
		switch( elf_isa ) {
		case 0x03:
			return ISA_X86;
		case 0x08:
			return ISA_MIPS;
		case 0x3E:
			return ISA_AMD64;
		case 0xF3:
			return ISA_RISC_V;
		default:
			return ISA_UNKNOWN;
		}
	}

	VanadisELFOSABI getOSABI() const {
		switch( elf_os_abi ) {
		case 0x03:
			return OS_ABI_LINUX;
		case 0x00:
			return OS_ABI_SYS_V;
		default:
			return OS_ABI_UNKNOWN;
		}
	}

	uint16_t getOSABIVersion() const { return elf_abi_version; }

	VanadisELFObjectType getObjectType() const {
		switch( elf_obj_type ) {
		case 0x01:
			return OBJ_TYPE_REL;
		case 0x02:
			return OBJ_TYPE_EXEC;
		case 0x03:
			return OBJ_TYPE_DYN;
		default:
			return OBJ_TYPE_UNKNOWN;
		}
	}

	void setBinaryPath( const char* new_path ) {
		bin_path = new char[ strlen(new_path) + 1 ];
		sprintf(bin_path, "%s", new_path);
	}

	const char* getBinaryPath() const { return bin_path; }

	uint64_t getEntryPoint() const { return elf_entry_point; }
	uint64_t getProgramHeaderOffset() const { return elf_prog_header_start; }
	uint64_t getSectionHeaderOffset() const { return elf_prog_section_start; }

	uint16_t getProgramHeaderEntrySize() const { return elf_prog_header_entry_size; }
	uint16_t getProgramHeaderEntryCount() const { return elf_prog_header_entry_count; }
	uint16_t getSectionHeaderEntrySize() const { return elf_prog_section_entry_size; }
	uint16_t getSectionHeaderEntryCount() const { return elf_prog_section_entry_count; }
	uint16_t getSectionHeaderNameEntryOffset() const { return elf_section_header_name_location; }

protected:
	char*    bin_path;
	uint8_t  elf_class;
	uint8_t  elf_endian;
	uint8_t  elf_os_abi;
	uint8_t  elf_abi_version;
	uint16_t elf_obj_type;
	uint16_t elf_isa;
	
	uint64_t elf_entry_point;
	uint64_t elf_prog_header_start;
	uint64_t elf_prog_section_start;

	uint16_t elf_prog_header_entry_size;
	uint16_t elf_prog_header_entry_count;
	uint16_t elf_prog_section_entry_size;
	uint16_t elf_prog_section_entry_count;
	uint16_t elf_section_header_name_location;	

};

VanadisELFInfo* readBinaryELFInfo( SST::Output* output, const char* path ) {
	
	FILE* bin_file = fopen( path, "rb" );

	if( nullptr == bin_file ) {
		output->fatal(CALL_INFO, -1, "Error: unable to open \'%s\', cannot read ELF table.\n", path);
	}

	uint8_t* elf_magic = new uint8_t[4];
	fread( elf_magic, 1, 4, bin_file );

	if( ! ( elf_magic[0] == 0x7F && elf_magic[1] == 0x45 && elf_magic[2] == 0x4c && elf_magic[3] == 0x46 ) ) {
		output->fatal(CALL_INFO, -1, "Error: opened %s, but the ELF magic header is not correct.\n", path );
	}

	VanadisELFInfo* elf_info = new VanadisELFInfo();

	elf_info->setBinaryPath( path );

	uint8_t tmp_byte   = 0;
	uint16_t tmp_2byte = 0;
	uint32_t tmp_4byte = 0;
	uint64_t tmp_8byte = 0;

	fread( &tmp_byte, 1, 1, bin_file );
	elf_info->setClass( tmp_byte );

	fread( &tmp_byte, 1, 1, bin_file );
	elf_info->setEndian( tmp_byte );
	
	fread( &tmp_byte, 1, 1, bin_file );
	// Discard this read, it is set to 1 for modern ELF

	fread( &tmp_byte, 1, 1, bin_file );
	elf_info->setOSABI( tmp_byte );

	fread( &tmp_byte, 1, 1, bin_file );
	elf_info->setOSABIVersion( tmp_byte );

	// Discard the next 7 bytes, these pad the header
	fread( &tmp_byte, 1, 1, bin_file);
	fread( &tmp_byte, 1, 1, bin_file);
	fread( &tmp_byte, 1, 1, bin_file);
	fread( &tmp_byte, 1, 1, bin_file);
	fread( &tmp_byte, 1, 1, bin_file);
	fread( &tmp_byte, 1, 1, bin_file);
	fread( &tmp_byte, 1, 1, bin_file);

	fread( &tmp_2byte, 2, 1, bin_file );
	elf_info->setObjectType( tmp_2byte );

	fread( &tmp_2byte, 2, 1, bin_file );
	elf_info->setISA( tmp_2byte );

	// Discard the next 4 bytes, these just set to 1 to pad
	fread( &tmp_byte, 1, 1, bin_file);
        fread( &tmp_byte, 1, 1, bin_file);
        fread( &tmp_byte, 1, 1, bin_file);
        fread( &tmp_byte, 1, 1, bin_file);

	if( elf_info->isELF64() ) {
		fread( &tmp_8byte, 8, 1, bin_file );
		elf_info->setEntryPoint( tmp_8byte );

		fread( &tmp_8byte, 8, 1, bin_file );
		elf_info->setProgramHeaderOffset( tmp_8byte );

		fread( &tmp_8byte, 8, 1, bin_file );
		elf_info->setSectionHeaderOffset( tmp_8byte );
	} else if( elf_info->isELF32() ) {
		fread( &tmp_4byte, 4, 1, bin_file );
		elf_info->setEntryPoint( (uint64_t) tmp_4byte );

		fread( &tmp_4byte, 4, 1, bin_file );
		elf_info->setProgramHeaderOffset( (uint64_t) tmp_4byte );

		fread( &tmp_4byte, 4, 1, bin_file );
		elf_info->setSectionHeaderOffset( (uint64_t) tmp_4byte );
	} else {
		output->fatal( CALL_INFO, -1, "Error: unable to determine if binary is 32/64 bits during ELF read.\n");
	}

	// Discard, ISA specific, need to understand whether we need this.
	fread( &tmp_4byte, 4, 1, bin_file );

	// Discard, is just a size read for ELF header info
	fread( &tmp_2byte, 2, 1, bin_file );

	fread( &tmp_2byte, 2, 1, bin_file );
	elf_info->setProgramHeaderEntrySize( tmp_2byte );

	fread( &tmp_2byte, 2, 1, bin_file );
	elf_info->setProgramHeaderEntryCount( tmp_2byte );

	fread( &tmp_2byte, 2, 1, bin_file );
	elf_info->setSectionHeaderEntrySize( tmp_2byte );

	fread( &tmp_2byte, 2, 1, bin_file );
	elf_info->setSectionHeaderEntryCount( tmp_2byte );

	fread( &tmp_2byte, 2, 1, bin_file );
	elf_info->setSectionEntryIndexForNames( tmp_2byte );	

	fclose( bin_file );
	return elf_info;
};

}
}

#endif
