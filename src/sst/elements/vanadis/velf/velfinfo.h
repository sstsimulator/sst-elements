
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
        SECTION_HEADER_REL,
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
	case SECTION_HEADER_REL:
		return "Relocation Information";
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
};

const char* getELFOSABIString( VanadisELFOSABI abi ) {
	switch( abi ) {
	case OS_ABI_LINUX:
		return "Linux";
	case OS_ABI_SYS_V:
		return "System-V";
	default:
		return "Unknown";
	}
};

enum VanadisSymbolType {
	SYMBOL_NO_TYPE,
	SYMBOL_OBJECT,
	SYMBOL_FUNCTION,
	SYMBOL_SECTION,
	SYMBOL_FILE,
	SYMBOL_UNKNOWN
};

enum VanadisSymbolBindType {
	SYMBOL_BIND_LOCAL,
	SYMBOL_BIND_GLOBAL,
	SYMBOL_BIND_WEAK,
	SYMBOL_BIND_UNKNOWN
};

const char* getSymbolBindTypeString( VanadisSymbolBindType bindT ) {
	switch( bindT ) {
	case SYMBOL_BIND_LOCAL:
		return "Local";
	case SYMBOL_BIND_GLOBAL:
		return "Global";
	case SYMBOL_BIND_WEAK:
		return "Weak";
	default:
		return "Unknown";
	}
};

const char* getSymbolTypeString( VanadisSymbolType symT ) {
	switch( symT ) {
	case SYMBOL_NO_TYPE:
		return "NO_TYPE";
	case SYMBOL_OBJECT:
		return "Object";
	case SYMBOL_FUNCTION:
		return "Function";
	case SYMBOL_SECTION:
		return "Section";
	case SYMBOL_FILE:
		return "File";
	default:
		return "Unknown";
	}
}

class VanadisELFRelocationEntry {
public:
	VanadisELFRelocationEntry() {
		rel_address = 0;
		symbol_info = 0;
	}

	void print( SST::Output* output, uint32_t index ) {
		output->verbose(CALL_INFO, 16, 0, "Relocation (index: %" PRIu32 " / address: %" PRIu64 " (0x%0llx) / info: %" PRIu64 "\n",
			index, rel_address, rel_address, symbol_info);
	}

	void setAddress( const uint64_t addr ) { rel_address = addr; }
	void setInfo( const uint64_t info    ) { symbol_info = info; }

	uint64_t getAddress() const { return rel_address; }
	uint64_t getInfo() const { return symbol_info; }

protected:
	uint64_t rel_address;
	uint64_t symbol_info;

};

class VanadisSymbolTableEntry {
public:
	VanadisSymbolTableEntry() {
		sym_name_offset = 0;
		sym_address     = 0;
		sym_size        = 0;
		sym_type        = SYMBOL_UNKNOWN;
		sym_bind        = SYMBOL_BIND_UNKNOWN;
		sym_sndx        = 0;
	}

	void setNameOffset( const uint64_t val ) { sym_name_offset = val; }
	void setAddress( const uint64_t val ) { sym_address = val; }
	void setSize( const uint64_t val ) { sym_size = val; }
	void setType( VanadisSymbolType newT ) { sym_type = newT; }
	void setBindType( VanadisSymbolBindType newBT ) { sym_bind = newBT; }
	void setName( const char* name_buffer ) {
		std::string name_buffer_str(name_buffer);
		symbolName = name_buffer_str;
	}
	void setSymbolSection( const uint64_t sec_in ) { sym_sndx = sec_in; }

	uint64_t getNameOffset() const { return sym_name_offset; }
	uint64_t getAddress() const { return sym_address; }
	uint64_t getSize() const { return sym_size; }
	const char* getName() const { return symbolName.c_str(); }
	VanadisSymbolType getType() const { return sym_type; }
	VanadisSymbolBindType getBindType() const { return sym_bind; }
	uint64_t getSymbolSection() const { return sym_sndx; }

	void print( SST::Output* output, size_t index ) {
		output->verbose(CALL_INFO, 32, 0, "Symbol (index: %" PRIu32 " / name: %s / offset: %" PRIu64 " / address: 0x%0llx / sz: %" PRIu64 ", type: %s / bind: %s / sndx: %" PRIu64 ")\n",
			(uint32_t) index, symbolName.c_str(), sym_name_offset, sym_address, sym_size, getSymbolTypeString( sym_type ),
			getSymbolBindTypeString( sym_bind ), sym_sndx );
	}

protected:
	std::string symbolName;
	VanadisSymbolBindType sym_bind;
	VanadisSymbolType sym_type;
	uint64_t sym_name_offset;
	uint64_t sym_address;
	uint64_t sym_size;
	uint64_t sym_sndx;

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

	void setHeaderTypeNum( uint64_t new_type ) { hdr_type_num = new_type; }
	void setHeaderType( VanadisELFProgramHeaderType new_type ) { hdr_type = new_type; }
	void setImageOffset( const uint64_t new_off ) { imgOffset = new_off; }
	void setVirtualMemoryStart( const uint64_t new_start ) { virtMemAddrStart = new_start; }
	void setPhysicalMemoryStart( const uint64_t new_start ) { physMemAddrStart = new_start; }
	void setHeaderImageLength( const uint64_t new_len ) { imgDataLen = new_len; }
	void setHeaderMemoryLength( const uint64_t new_len ) { memDataLen = new_len; }
	void setAlignment( const uint64_t new_align ) { alignment = new_align; }
	void setSegmentFlags( const uint64_t new_flags ) { segment_flags = new_flags; }

	VanadisELFProgramHeaderType getHeaderType() const { return hdr_type; }
	uint64_t getHeaderTypeNumber() const { return hdr_type_num; }
	uint64_t getImageOffset() const { return imgOffset; }
	uint64_t getVirtualMemoryStart() const { return virtMemAddrStart; }
	uint64_t getPhysicalMemoryStart() const { return physMemAddrStart; }
	uint64_t getHeaderImageLength() const { return imgDataLen; }
	uint64_t getHeaderMemoryLength() const { return memDataLen; }
	uint64_t getAlignment() const { return alignment; }
	uint64_t getSegmentFlags() const { return segment_flags; }

	void print( SST::Output* output, uint64_t index ) {
		output->verbose(CALL_INFO, 16, 0, ">> Program Header Entry:    %" PRIu64 "\n", index );
		output->verbose(CALL_INFO, 16, 0, "---> Header Type:           %" PRIu64 " / 0x%llx / %s\n",
			hdr_type_num, hdr_type_num, getELFProgramHeaderTypeString(hdr_type));
		output->verbose(CALL_INFO, 16, 0, "---> Image Offset:          %" PRIu64 "\n", imgOffset);
		output->verbose(CALL_INFO, 16, 0, "---> Virtual Memory Start:  %" PRIu64 " / %p\n", virtMemAddrStart, (void*) virtMemAddrStart);
		output->verbose(CALL_INFO, 16, 0, "---> Phys. Memory Start:    %" PRIu64 " / %p\n", physMemAddrStart, (void*) physMemAddrStart);
		output->verbose(CALL_INFO, 16, 0, "---> Image Data Length:     %" PRIu64 " / %p\n", imgDataLen, (void*) imgDataLen);
		output->verbose(CALL_INFO, 16, 0, "---> Image Mem Length:      %" PRIu64 " / %p\n", memDataLen, (void*) memDataLen);
		output->verbose(CALL_INFO, 16, 0, "---> Flags:                 %" PRIu64 " / 0x%0llx\n", segment_flags, segment_flags );
		output->verbose(CALL_INFO, 16, 0, "---> Alignment:             %" PRIu64 " / %p\n", alignment, (void*) alignment);
	}
private:
	void init() {
		hdr_type = PROG_HEADER_NOT_USED;
		imgOffset = 0;
		virtMemAddrStart = 0;
		physMemAddrStart = 0;
		imgDataLen = 0;
		memDataLen = 0;
		alignment = 0;
		hdr_type_num = 0;
		segment_flags = 0;
	}

	VanadisELFProgramHeaderType hdr_type;
	uint64_t hdr_type_num;
	uint64_t imgOffset;
	uint64_t virtMemAddrStart;
	uint64_t physMemAddrStart;
	uint64_t imgDataLen;
	uint64_t memDataLen;
	uint64_t alignment;
	uint64_t segment_flags;
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
	void setID( const uint64_t new_id ) { id = new_id; }

	VanadisELFSectionHeaderType getSectionType() const { return sec_type; }

	bool isWriteable()    const { return (sec_flags & 0x1  ) != 0; }
	bool isAllocated()    const { return (sec_flags & 0x2  ) != 0; }
	bool isExecutable()   const { return (sec_flags & 0x4  ) != 0; }
	bool isMergable()     const { return (sec_flags & 0x10 ) != 0; }
	bool isNullTerminatedStrings() const { return (sec_flags & 0x20 ) != 0; }
	bool containsSHTIndex() const { return (sec_flags & 0x40 ) != 0; }
	bool containsTLSData() const { return (sec_flags & 0x400 ) != 0; }

	uint64_t getID() const { return id; }
	uint64_t getVirtualMemoryStart() const { return virtMemAddrStart; }
	uint64_t getImageOffset() const { return imgOffset; }
	uint64_t getImageLength() const { return imgDataLen; }
	uint64_t getAlignment() const { return alignment; }

	void print( SST::Output* output, uint64_t index ) {
		output->verbose(CALL_INFO, 16, 0, ">> Section Entry %" PRIu64 " (id: %" PRIu64 ")\n", index, id);
		output->verbose(CALL_INFO, 16, 0, "---> Header Type:                 %s\n",
			getELFSectionHeaderTypeString( sec_type ));
		output->verbose(CALL_INFO, 16, 0, "---> Section Flags:               %" PRIu64 " / %p\n",
			sec_flags, (void*) sec_flags);
		output->verbose(CALL_INFO, 16, 0, "-----> isWriteable():             %s\n", isWriteable() ? "yes" : "no"  );
		output->verbose(CALL_INFO, 16, 0, "-----> isAllocated():             %s\n", isAllocated() ? "yes" : "no"  );
		output->verbose(CALL_INFO, 16, 0, "-----> isExecutable():            %s\n", isExecutable() ? "yes" : "no" );
 		output->verbose(CALL_INFO, 16, 0, "-----> isMergeable():             %s\n", isMergable() ? "yes" : "no"   );
		output->verbose(CALL_INFO, 16, 0, "-----> isNullTermStrings():       %s\n", isNullTerminatedStrings() ? "yes" : "no" );
		output->verbose(CALL_INFO, 16, 0, "-----> SHT-Index():               %s\n", containsSHTIndex() ? "yes" : "no" );
		output->verbose(CALL_INFO, 16, 0, "-----> TLS-Data():                %s\n", containsTLSData() ? "yes" : "no" );
		output->verbose(CALL_INFO, 16, 0, "---> Virtual Address:             %" PRIu64 " / %p\n", virtMemAddrStart,
			(void*) virtMemAddrStart);
		output->verbose(CALL_INFO, 16, 0, "---> Image Offset:                %" PRIu64 " / %p\n",
			imgOffset, (void*) imgOffset);
		output->verbose(CALL_INFO, 16, 0, "---> Image Data Length:           %" PRIu64 " / %p\n",
			imgDataLen, (void*) imgDataLen);
		output->verbose(CALL_INFO, 16, 0, "---> Alignment:                   %" PRIu64 " / %p\n",
			alignment, (void*) alignment);
	}

private:
	void init() {
		sec_type = SECTION_HEADER_NOT_USED;
		sec_flags = 0;
		virtMemAddrStart = 0;
		imgOffset = 0;
		imgDataLen = 0;
		alignment = 0;
		id = 0;
	}

	uint64_t id;
	VanadisELFSectionHeaderType sec_type;
	uint64_t sec_flags;
	uint64_t virtMemAddrStart;
	uint64_t imgOffset;
	uint64_t imgDataLen;
	uint64_t alignment;

};

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
		output->verbose(CALL_INFO, 2, 0, "Sect-hdr Offset:      %p\n",
 			(void*) getSectionHeaderOffset() );
		output->verbose(CALL_INFO, 2, 0, "Sect-Hdr Entry Size:  %" PRIu16 "\n",
			getSectionHeaderEntrySize() );
		output->verbose(CALL_INFO, 2, 0, "Sect-Hdr Entry Count: %" PRIu16 "\n",
			getSectionHeaderEntryCount() );
		output->verbose(CALL_INFO, 2, 0, "Sect-Hdr Strings Loc: %" PRIu16 "\n",
			getSectionHeaderNameEntryOffset() );
		output->verbose(CALL_INFO, 2, 0, "Symbols Found:        %" PRIu32 "\n",
			(uint32_t) progSymbols.size() );
		output->verbose(CALL_INFO, 2, 0, "Relocation Entries:   %" PRIu32 "\n",
			(uint32_t) progRelDyn.size() );
		output->verbose(CALL_INFO, 2, 0, "-------------------------------------------------------\n");

		for( int i = 0; i < progHeaders.size(); ++i ) {
			progHeaders[i]->print( output, i );
			output->verbose(CALL_INFO, 16, 0, "-------------------------------------------------------\n");
		}

		for( int i = 0; i < progSections.size(); ++i ) {
			progSections[i]->print( output, i );
			output->verbose(CALL_INFO, 16, 0, "-------------------------------------------------------\n");
		}

		for( int i = 0; i < progSymbols.size(); ++i ) {
			progSymbols[i]->print( output, i );
		}

		for( int i = 0; i < progRelDyn.size(); ++i ) {
			progRelDyn[i]->print( output, i );
		}

		output->verbose(CALL_INFO, 16, 0, "-------------------------------------------------------\n");
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

	size_t countSymbols() const { return progSymbols.size(); }
	const VanadisSymbolTableEntry* getSymbol( const size_t index ) {
		return progSymbols[index];
	}

	size_t countDynRelocations() const { return progRelDyn.size(); }
	const VanadisELFRelocationEntry* getDynRelocation( const size_t index ) {
		return progRelDyn[index];
	}

	void addProgramHeader( VanadisELFProgramHeaderEntry* new_header ) {
		progHeaders.push_back(new_header);
	}

	void addProgramSection( VanadisELFProgramSectionEntry* new_sec ) {
		progSections.push_back(new_sec);
	}

	void addSymbolTableEntry( VanadisSymbolTableEntry* new_entry ) {
		progSymbols.push_back( new_entry );
	}

	void addRelocationEntry( VanadisELFRelocationEntry* new_rel ) {
		progRelDyn.push_back( new_rel );
	}

	bool isDynamicExecutable() const {
		bool found_interp = false;

		for( VanadisELFProgramHeaderEntry* next_entry : progHeaders ) {
			if( PROG_HEADER_INTERPRETER == next_entry->getHeaderType() ) {
				found_interp = true;
				break;
			}
		}

		return found_interp;
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

		for( VanadisSymbolTableEntry* next_sym : progSymbols ) {
			delete next_sym;
		}

		for( VanadisELFRelocationEntry* next_rel : progRelDyn ) {
			delete next_rel;
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
	std::vector<VanadisSymbolTableEntry*>       progSymbols;
	std::vector<VanadisELFRelocationEntry*>     progRelDyn;
};

void readString( FILE* bin_file, uint64_t stringTableStart, uint64_t stringTableOffset, std::vector<char>& nameBuffer ) {
	uint64_t current_file_pos = (uint64_t) ftell( bin_file );

	nameBuffer.clear();
	fseek( bin_file, stringTableStart + stringTableOffset, SEEK_SET );

	while( ! feof(bin_file) ) {
		int next_c = fgetc( bin_file );

		if( '\0' == next_c ) {
			break;
		} else if( EOF == next_c ) {
			break;
		}

		nameBuffer.push_back( (char) next_c );
	}

	// Push back a null character as we are turning this into a string later
	nameBuffer.push_back('\0');

	// Restore file position
	fseek( bin_file, current_file_pos, SEEK_SET );
}

void readBinarySymbolTable( SST::Output* output, const char* path, VanadisELFInfo* elf_info,
	const VanadisELFProgramSectionEntry* symbolSection,
	const VanadisELFProgramSectionEntry* stringTableEntry ) {

	FILE* bin_file = fopen( path, "rb" );

	if( nullptr == bin_file ) {
		output->fatal(CALL_INFO, -1, "Error: unable to open %s for reading symbol table.\n", path);
	}

	const bool binary_is_32 = elf_info->isELF32();

	output->verbose(CALL_INFO, 16, 0, "Symbol Table located at %" PRIu64 " / 0x%0llx in executable (is 32bit? %s).\n",
		symbolSection->getImageOffset(), symbolSection->getImageOffset(),
		binary_is_32 ? "yes" : "no" );

	// Wind forward to the symbol table entries
	fseek( bin_file, symbolSection->getImageOffset(), SEEK_SET );
	uint64_t symbol_size = binary_is_32 ? (4+4+4+2+2) : (4+2+2+8+8);

	output->verbose(CALL_INFO, 16, 0, "Symbol entry is %" PRIu64 " bytes in size, expecting %" PRIu64 " symbols (remainder: %" PRIu64 ")\n",
		symbol_size, (symbolSection->getImageLength() / symbol_size),
		symbolSection->getImageLength() % symbol_size);

	uint8_t  tmp_u8  = 0;
	uint16_t tmp_u16 = 0;
	uint32_t tmp_u32 = 0;
	uint64_t tmp_u64 = 0;

	std::vector<char> name_buffer;

	if( binary_is_32 ) {

	} else {
		output->fatal(CALL_INFO, -1, "Not implemented yet - 64-bit symbol reads are not yet implemented.\n");
	}

	for( uint64_t i = symbolSection->getImageOffset() ; i < (symbolSection->getImageOffset() + symbolSection->getImageLength());
		i += symbol_size ) {

		VanadisSymbolTableEntry* new_symbol = new VanadisSymbolTableEntry();

		if( binary_is_32 ) {
			fread( &tmp_u32, 4, 1, bin_file );
			new_symbol->setNameOffset( tmp_u32 );

			fread( &tmp_u32, 4, 1, bin_file );
			new_symbol->setAddress( tmp_u32 );

			fread( &tmp_u32, 4, 1, bin_file );
			new_symbol->setSize( tmp_u32 );

			fread( &tmp_u8, 1, 1, bin_file );
			//output->verbose(CALL_INFO, 16, 0, "Symbol info >> 4 = %" PRIu64 "\n", (tmp_u8 >> 4));

			switch( (tmp_u8 >> 4) ) {
			case 0:
				new_symbol->setBindType( SYMBOL_BIND_LOCAL );
				break;
			case 1:
				new_symbol->setBindType( SYMBOL_BIND_GLOBAL );
				break;
			case 2:
				new_symbol->setBindType( SYMBOL_BIND_WEAK );
				break;
			default:
				break;
			}

			//output->verbose(CALL_INFO, 16, 0, "Symbol type = %" PRIu64 "\n", (tmp_u8 & 0xf) );
			switch( (tmp_u8 & 0xf) ) {
			case 0:
				new_symbol->setType( SYMBOL_NO_TYPE );
				break;
			case 1:
				new_symbol->setType( SYMBOL_OBJECT );
				break;
			case 2:
				new_symbol->setType( SYMBOL_FUNCTION );
				break;
			case 3:
				new_symbol->setType( SYMBOL_SECTION );
				break;
			case 4:
				new_symbol->setType( SYMBOL_FILE );
				break;
			default:
				break;
			}

			fread( &tmp_u8, 1, 1, bin_file );
			fread( &tmp_u16, 2, 1, bin_file );

			new_symbol->setSymbolSection( tmp_u16 );

			if( nullptr != stringTableEntry ) {
				if( new_symbol->getNameOffset() > 0 ) {
					readString( bin_file, stringTableEntry->getImageOffset(), new_symbol->getNameOffset(), name_buffer );
					new_symbol->setName( &name_buffer[0] );
				} else {
					const char* no_symbol_name = "";
					new_symbol->setName( no_symbol_name );
				}
			}

			elf_info->addSymbolTableEntry( new_symbol );
		} else {

		}
	}


	fclose( bin_file );
}

void readELFRelocationInformation( SST::Output* output, const char* path, VanadisELFInfo* elf_info,
	const VanadisELFProgramSectionEntry* relocationEntry ) {

	FILE* bin_file = fopen( path, "rb" );

	if( nullptr == bin_file ) {
		output->fatal(CALL_INFO, -1, "Error - unable to open %s\n", path);
	}

	fseek( bin_file, relocationEntry->getImageOffset(), SEEK_SET );

	if( elf_info->isELF32() ) {
		uint32_t u32_tmp = 0;

		for( size_t i = relocationEntry->getImageOffset(); i < (relocationEntry->getImageOffset() + relocationEntry->getImageLength());
			i += 8) {

			VanadisELFRelocationEntry* new_reloc = new VanadisELFRelocationEntry();

			fread( &u32_tmp, 4, 1, bin_file );
			new_reloc->setAddress( u32_tmp );

			fread( &u32_tmp, 4, 1, bin_file );
			new_reloc->setInfo( u32_tmp );

			elf_info->addRelocationEntry( new_reloc );
		}
	} else {
		output->fatal(CALL_INFO, -1, "64bit relocation entry reads are not implemented yet.\n");
	}


	fclose( bin_file );
}

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

	const uint32_t prog_header_count = (uint32_t) tmp_2byte;

	fread( &tmp_2byte, 2, 1, bin_file );
	elf_info->setSectionHeaderEntrySize( tmp_2byte );

	fread( &tmp_2byte, 2, 1, bin_file );
	elf_info->setSectionHeaderEntryCount( tmp_2byte );

	fread( &tmp_2byte, 2, 1, bin_file );
	elf_info->setSectionEntryIndexForNames( tmp_2byte );

	// Wind the file pointer to the program header offset
	fseek( bin_file, elf_info->getProgramHeaderOffset(), SEEK_SET);

	for( uint32_t i = 0; i < prog_header_count; ++i ) {
		output->verbose(CALL_INFO, 4, 0, "Reading Program Header %" PRIu32 "...\n", i);
		VanadisELFProgramHeaderEntry* new_prg_hdr = new VanadisELFProgramHeaderEntry();

		// Header type
		fread( &tmp_4byte, 4, 1, bin_file );

		VanadisELFProgramHeaderType prg_hdr_type = PROG_HEADER_NOT_USED;

		switch( tmp_4byte ) {
		case 0x0: prg_hdr_type = PROG_HEADER_NOT_USED;     break;
		case 0x1: prg_hdr_type = PROG_HEADER_LOAD;         break;
		case 0x2: prg_hdr_type = PROG_HEADER_DYNAMIC;      break;
		case 0x3: prg_hdr_type = PROG_HEADER_INTERPRETER;  break;
		case 0x4: prg_hdr_type = PROG_HEADER_NOTE;         break;
		case 0x5: /* not used? */ break;
		case 0x6: prg_hdr_type = PROG_HEADER_TABLE_INFO;   break;
		case 0x7: prg_hdr_type = PROG_HEADER_THREAD_LOCAL; break;
		default:
			output->verbose(CALL_INFO, 4, 0, "Unknown program header type in ELF: %" PRIu32 "\n", tmp_4byte );
		}

		new_prg_hdr->setHeaderTypeNum( tmp_4byte );
		new_prg_hdr->setHeaderType( prg_hdr_type );

		if( elf_info->isELF64() ) {
			// Discard segment-dependent flags
			fread( &tmp_4byte, 4, 1, bin_file );

			fread( &tmp_8byte, 8, 1, bin_file);
			new_prg_hdr->setImageOffset( tmp_8byte );

			fread( &tmp_8byte, 8, 1, bin_file);
			new_prg_hdr->setVirtualMemoryStart( tmp_8byte );

			fread( &tmp_8byte, 8, 1, bin_file);
			new_prg_hdr->setPhysicalMemoryStart( tmp_8byte );

			fread( &tmp_8byte, 8, 1, bin_file);
			new_prg_hdr->setHeaderImageLength( tmp_8byte );

			fread( &tmp_8byte, 8, 1, bin_file);
			new_prg_hdr->setHeaderMemoryLength( tmp_8byte );

			fread( &tmp_8byte, 8, 1, bin_file);
			new_prg_hdr->setAlignment( tmp_8byte );
		} else if( elf_info->isELF32() ) {
			fread( &tmp_4byte, 4, 1, bin_file);
			new_prg_hdr->setImageOffset( tmp_4byte );

			fread( &tmp_4byte, 4, 1, bin_file);
			new_prg_hdr->setVirtualMemoryStart( tmp_4byte );

			fread( &tmp_4byte, 4, 1, bin_file);
			new_prg_hdr->setPhysicalMemoryStart( tmp_4byte );

			fread( &tmp_4byte, 4, 1, bin_file);
			new_prg_hdr->setHeaderImageLength( tmp_4byte );

			fread( &tmp_4byte, 4, 1, bin_file);
			new_prg_hdr->setHeaderMemoryLength( tmp_4byte );

			// Segment dependent flags - ignore?
			fread( &tmp_4byte, 4, 1, bin_file);
			new_prg_hdr->setSegmentFlags( tmp_4byte );

			fread( &tmp_4byte, 4, 1, bin_file);
			new_prg_hdr->setAlignment( tmp_4byte );
		} else {
			output->fatal(CALL_INFO, -1, "Error: neither 32 or 64 bit test passed ELF read.\n");
		}

		elf_info->addProgramHeader( new_prg_hdr );
	}

	fseek( bin_file, elf_info->getSectionHeaderOffset(), SEEK_SET );

	for( uint32_t i = 0; i < elf_info->getSectionHeaderEntryCount(); ++i ) {
		output->verbose(CALL_INFO, 4, 0, "Reading Section Header %" PRIu32 "...\n", i);
		VanadisELFProgramSectionEntry* new_sec = new VanadisELFProgramSectionEntry();
		new_sec->setID( i );

		// Offset for section name
		fread( &tmp_4byte, 4, 1, bin_file );

		fread( &tmp_4byte, 4, 1, bin_file );
		VanadisELFSectionHeaderType sec_type = SECTION_HEADER_NOT_USED;

		switch( tmp_4byte ) {
		case 0x0:  sec_type = SECTION_HEADER_NOT_USED;                  break;
		case 0x1:  sec_type = SECTION_HEADER_PROG_DATA;		   	break;
		case 0x2:  sec_type = SECTION_HEADER_SYMBOL_TABLE;		break;
		case 0x3:  sec_type = SECTION_HEADER_STRING_TABLE;		break;
		case 0x4:  sec_type = SECTION_HEADER_RELOCATABLE_ENTRY;		break;
		case 0x5:  sec_type = SECTION_HEADER_SYMBOL_HASH_TABLE;		break;
		case 0x6:  sec_type = SECTION_HEADER_DYN_LINK_INFO;		break;
		case 0x7:  sec_type = SECTION_HEADER_NOTE;			break;
		case 0x8:  sec_type = SECTION_HEADER_BSS;			break;
		case 0x9:  sec_type = SECTION_HEADER_REL;			break;
		case 0x0B: sec_type = SECTION_HEADER_DYN_SYMBOL_INFO;		break;
		case 0x0E: sec_type = SECTION_HEADER_INIT_ARRAY;		break;
		case 0x0F: sec_type = SECTION_HEADER_FINI_ARRAY;		break;
		case 0x10: sec_type = SECTION_HEADER_PREINIT_ARRAY;		break;
		case 0x11: sec_type = SECTION_HEADER_SECTION_GROUP;		break;
		case 0x12: sec_type = SECTION_HEADER_EXTENDED_SECTION_INDEX;	break;
		case 0x13: sec_type = SECTION_HEADER_DEFINED_TYPES;		break;
		default:
			output->verbose(CALL_INFO, 4, 0, "Unknown Section type: %" PRIu32 "\n", tmp_4byte );
			break;
		}

		new_sec->setSectionType( sec_type );

		if( elf_info->isELF64() ) {
			fread( &tmp_8byte, 8, 1, bin_file );
			new_sec->setSectionFlags( tmp_8byte );

			fread( &tmp_8byte, 8, 1, bin_file );
			new_sec->setVirtualMemoryStart( tmp_8byte );

			fread( &tmp_8byte, 8, 1, bin_file );
			new_sec->setImageOffset( tmp_8byte );

			fread( &tmp_8byte, 8, 1, bin_file );
			new_sec->setImageLength( tmp_8byte );

			fread( &tmp_4byte, 4, 1, bin_file );
			fread( &tmp_4byte, 4, 1, bin_file );

			fread( &tmp_8byte, 8, 1, bin_file );
			new_sec->setAlignment( tmp_8byte );

			fread( &tmp_8byte, 8, 1, bin_file );
		} else if( elf_info->isELF32() ) {
			fread( &tmp_4byte, 4, 1, bin_file );
			new_sec->setSectionFlags( tmp_4byte );

			fread( &tmp_4byte, 4, 1, bin_file );
			new_sec->setVirtualMemoryStart( tmp_4byte );

			fread( &tmp_4byte, 4, 1, bin_file );
			new_sec->setImageOffset( tmp_4byte );

			fread( &tmp_4byte, 4, 1, bin_file );
			new_sec->setImageLength( tmp_4byte );

			fread( &tmp_4byte, 4, 1, bin_file );
			fread( &tmp_4byte, 4, 1, bin_file );

			fread( &tmp_4byte, 4, 1, bin_file );
			new_sec->setAlignment( tmp_4byte );

			fread( &tmp_4byte, 4, 1, bin_file );
		} else {
			output->fatal(CALL_INFO, -1, "Error: not 32 or 64-bit type ELF info. Not sure what to do.\n");
		}

		elf_info->addProgramSection( new_sec );
	}

	fclose( bin_file );

	VanadisELFProgramSectionEntry* string_table_entry = nullptr;

	for( size_t i = 0; i < elf_info->countProgramSections(); ++i ) {
		const VanadisELFProgramSectionEntry* next_entry = elf_info->getProgramSection( i );

		if( next_entry->getSectionType() == SECTION_HEADER_STRING_TABLE ) {
			string_table_entry = const_cast<VanadisELFProgramSectionEntry*>(next_entry);
			break;
		}
	}

	for( size_t i = 0; i < elf_info->countProgramSections(); ++i ) {
		const VanadisELFProgramSectionEntry* next_entry = elf_info->getProgramSection( i );

		if( next_entry->getSectionType() == SECTION_HEADER_SYMBOL_TABLE ) {
			output->verbose(CALL_INFO, 16, 0, "Reading in symbol table (Section %" PRIu64 ")...\n",
				next_entry->getID());

			readBinarySymbolTable( output, path, elf_info, next_entry, string_table_entry );

			output->verbose(CALL_INFO, 16, 0, "Read of section completed.\n");
		} else if( next_entry->getSectionType() == SECTION_HEADER_REL ) {
			output->verbose(CALL_INFO, 16, 0, "Reading in relocation table (Section %" PRIu64 ")...\n",
				next_entry->getID());

			readELFRelocationInformation( output, path, elf_info, next_entry );

			output->verbose(CALL_INFO, 16, 0, "Read of relocation entry completed\n");
		}
	}

	return elf_info;
};



}
}

#endif
