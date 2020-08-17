
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

enum VanadisELFProgramHeaderType {
	PROG_HEADER_NOT_USED,
	PROG_HEADER_LOAD,
	PROG_HEADER_DYNAMIC,
	PROG_HEADER_INTERPRETER,
	PROG_HEADER_NOTE,
	PROG_HEADER_TABLE_INFO,
	PROG_HEADER_THREAD_LOCAL
};

enum VanadisELFSectionHeaderType {
	SECTION_HEADER_NOT_USED,
	SECTION_HEADER_PROG_DATA,
	SECTION_HEADER_SYMBOL_TABLE,
	SECTION_HEADER_STRING_TABLE,
	SECTION_HEADER_RELOCATABLE_ENTRY,
	SECTION_HEADER_SYMBOL_HASH_TABLE,
	SECTION_HEADER_DYN_LINK_INFO,
	SECTION_HEADER_NOTE,
	SECTION_HEADER_BSS,
	SECTION_HEADER_DYN_SYMBOL_INFO,
	SECTION_HEADER_INIT_ARRAY,
	SECTION_HEADER_FINI_ARRAY,
	SECTION_HEADER_PREINIT_ARRAY,
	SECTION_HEADER_SECTION_GROUP,
	SECTION_HEADER_EXTENDED_SECTION_INDEX,
	SECTION_HEADER_DEFINED_TYPES
};

const char* getELFSectionHeaderTypeString( VanadisELFSectionHeaderType sec_type ) {
	switch( sec_type ) {
	case SECTION_HEADER_NOT_USED:
		return "NOT_USED";
	case SECTION_HEADER_PROG_DATA:
		return "Program Data";
	case SECTION_HEADER_SYMBOL_TABLE:
		return "Program Symbol Table";
	case SECTION_HEADER_STRING_TABLE:
		return "Program String Table";
	case SECTION_HEADER_RELOCATABLE_ENTRY:
		return "Relocatable Entry";
	case SECTION_HEADER_SYMBOL_HASH_TABLE:
		return "Symbol Hash Table";
	case SECTION_HEADER_DYN_LINK_INFO:
		return "Dynamic Linking Information";
	case SECTION_HEADER_NOTE:
		return "Notes";
	case SECTION_HEADER_BSS:
		return "Program Space without Data (BSS Entry)";
	case SECTION_HEADER_DYN_SYMBOL_INFO:
		return "Dynamic Linker Symbol Table";
	case SECTION_HEADER_INIT_ARRAY:
		return "Array of Constructors";
	case SECTION_HEADER_FINI_ARRAY:
		return "Array of Destructors";
	case SECTION_HEADER_PREINIT_ARRAY:
		return "Array of Pre-constructors";
	case SECTION_HEADER_SECTION_GROUP:
		return "Section Group";
	case SECTION_HEADER_EXTENDED_SECTION_INDEX:
		return "Extended Section Indices";
	case SECTION_HEADER_DEFINED_TYPES:
		return "Number of Defined Types in Section Header";
	default:
		return "Unknown";
	}
};

const char* getELFProgramHeaderTypeString( VanadisELFProgramHeaderType hdr ) {
	switch( hdr ) {
	case PROG_HEADER_NOT_USED:
		return "NOT_USED";
	case PROG_HEADER_LOAD:
		return "Loadable Segment";
	case PROG_HEADER_DYNAMIC:
		return "Dynamic Linking Info";
	case PROG_HEADER_INTERPRETER:
		return "Interpreter Information";
	case PROG_HEADER_NOTE:
		return "Auxillary ELF Information";
	case PROG_HEADER_TABLE_INFO:
		return "ELF Program Table Information";
	case PROG_HEADER_THREAD_LOCAL:
		return "Thread Local Storage Template";
	default:
		return "Unknown";
	}
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
};

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

	size_t countProgramHeaders() const { return progHeaders.size(); }
	const VanadisELFProgramHeaderEntry* getProgramHeader( const size_t index ) {
		return progHeaders[index];
	}

	size_t countProgramSections() const { return progSections.size(); }
	const VanadisELFProgramSectionEntry* getProgramSection( const size_t index ) {
		return progSections[index];
	}

	void addProgramHeader( VanadisELFProgramHeaderEntry* new_header ) {
		progHeaders.push_back(new_header);
	}

	void addProgramSection( VanadisELFProgramSectionEntry* new_sec ) {
		progSections.push_back(new_sec);
	}

	~VanadisELFInfo() {
		if( nullptr != bin_path ) {
			delete[] bin_path;
		}

		for( VanadisELFProgramHeaderEntry* next_hdr : progHeaders ) {
			delete next_hdr;
		}

		for( VanadisELFProgramSectionEntry* next_sec : progSections ) {
			delete next_sec;
		}
	}

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

	std::vector<VanadisELFProgramHeaderEntry*>  progHeaders;
	std::vector<VanadisELFProgramSectionEntry*> progSections;
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

class VanadisELFProgramHeaderEntry {
public:
	VanadisELFProgramHeaderEntry( VanadisELFProgramHeaderType hdr ) {
		init();
		hdr_type = hdr;
	}

	VanadisELFProgramHeaderEntry() {
		init();
	}

	void setHeaderType( VanadisELFProgramHeaderType new_type ) { hdr_type = new_type; }
	void setImageOffset( const uint64_t new_off ) { imgOffset = new_off; }
	void setVirtualMemoryStart( const uint64_t new_start ) { virtMemAddrStart = new_start; }
	void setPhysicalMemoryStart( const uint64_t new_start ) { physMemAddrStart = new_start; }
	void setHeaderImageLength( const uint64_t new_len ) { imgDataLen = new_len; }
	void setHeaderMemoryLength( const uint64_t new_len ) { memDataLen = new_len; }
	void setAlignment( const uint64_t new_align ) { alignment = new_align; }

	VanadisELFProgramHeaderType getHeaderType() const { return hdr_type; }
	uint64_t getImageOffset() const { return imgOffset; }
	uint64_t getVirtualMemoryStart() const { return virtMemAddrStart; }
	uint64_t getPhysicalMemoryStart() const { return physMemAddrStart; }
	uint64_t getHeaderImageLength() const { return imgDataLen; }
	uint64_t getHeaderMemoryLength() const { return memDataLen; }
	uint64_t getAlignment() const { return alignment; }

private:
	void init() {
		hdr_type = PROG_HEADER_NOT_USED;
		imgOffset = 0;
		virtMemAddrStart = 0;
		physMemAddrStart = 0;
		imgDataLen = 0;
		memDataLen = 0;
		alignment = 0;
	}

	VanadisELFProgramHeaderType hdr_type;
	uint64_t imgOffset;
	uint64_t virtMemAddrStart;
	uint64_t physMemAddrStart;
	uint64_t imgDataLen;
	uint64_t memDataLen;
	uint64_t alignment;
};

class VanadisELFProgramSectionEntry {
public:
	VanadisELFProgramSectionEntry() {
		init();
	}

	VanadisELFProgramSectionEntry( VanadisELFSectionHeaderType s_type ) {
		init();
		sec_type = s_type;
	}

	void setSectionType( VanadisELFSectionHeaderType new_type ) { sec_type = new_type; }
	void setSectionFlags( const uint64_t new_flags ) { sec_flags = new_flags; }
	void setVirtualMemoryStart( const uint64_t new_start ) { virtMemAddrStart = new_start; }
	void setImageOffset( const uint64_t new_off ) { imgOffset = new_off; }
	void setImageLength( const uint64_t new_len ) { imgDataLen = new_len; }
	void setAlignment( const uint64_t new_align ) { alignment = new_align; }

	VanadisELFSectionHeaderType getSectionType() const { return sec_type; }

	bool isWriteable()    const { return (sec_flags & 0x1  ) != 0; }
	bool isAllocated()    const { return (sec_flags & 0x2  ) != 0; }
	bool isExecutable()   const { return (sec_flags & 0x4  ) != 0; }
	bool isMergable()     const { return (sec_flags & 0x10 ) != 0; }
	bool isNullTerminatedStrings() const { return (sec_flags & 0x20 ) != 0; }
	bool containsSHTIndex() const { return (sec_flags & 0x40 ) != 0; }
	bool containsTLSData() const { return (sec_flags & 0x400 ) != 0; }

	uint64_t getVirtualMemoryStart() const { return virtMemAddrStart; }
	uint64_t getImageOffset() const { return imgOffset; }
	uint64_t getImageLength() const { return imgDataLen; }
	uint64_t getAlignment() const { return alignment; }

private:
	void init() {
		sec_type = SECTION_HEADER_NOT_USED;
		sec_flags = 0;
		virtMemAddrStart = 0;
		imgOffset = 0;
		imgDataLen = 0;
		alignment = 0;
	}

	VanadisELFSectionHeaderType sec_type;
	uint64_t sec_flags;
	uint64_t virtMemAddrStart;
	uint64_t imgOffset;
	uint64_t imgDataLen;
	uint64_t alignment;

};

}
}

#endif
