#include <stdint.h>
#include <string>
#include "os/velfloader.h"

using namespace SST::Vanadis;
namespace SST {
namespace Vanadis {


uint64_t loadElfFile( SST::Output* output, SST::Interfaces::StandardMem* mem_if, VanadisELFInfo* elf_info ) {


    output->verbose( CALL_INFO, 2, 0, "-> Loading %s, to locate program sections ...\n", elf_info->getBinaryPath());
    FILE* exec_file = fopen(elf_info->getBinaryPath(), "rb");

    if ( nullptr == exec_file ) {
        output->fatal(CALL_INFO, -1, "Error: unable to open %s\n", elf_info->getBinaryPath());
    }

    std::vector<uint8_t> initial_mem_contents;
    uint64_t             max_content_address = 0;

    // Find the max value we think we are going to need to place entries up
    // to

    for ( size_t i = 0; i < elf_info->countProgramHeaders(); ++i ) {
        const VanadisELFProgramHeaderEntry* next_prog_hdr = elf_info->getProgramHeader(i);
        max_content_address = 
            std::max( max_content_address, (uint64_t)next_prog_hdr->getVirtualMemoryStart() + next_prog_hdr->getHeaderImageLength());
    }

    for ( size_t i = 0; i < elf_info->countProgramSections(); ++i ) {
        const VanadisELFProgramSectionEntry* next_sec = elf_info->getProgramSection(i);
        max_content_address = 
            std::max( max_content_address, (uint64_t)next_sec->getVirtualMemoryStart() + next_sec->getImageLength());
    }

    output->verbose( CALL_INFO, 2, 0, "-> expecting max address for initial binary load is " "0x%llx, zeroing the memory\n", max_content_address);
    initial_mem_contents.resize(max_content_address, (uint8_t)0);

    // Populate the memory with contents from the binary
    output->verbose(CALL_INFO, 2, 0, "-> populating memory contents with info from the executable...\n");

    for ( size_t i = 0; i < elf_info->countProgramSections(); ++i ) {
        const VanadisELFProgramSectionEntry* next_sec = elf_info->getProgramSection(i);

        if ( SECTION_HEADER_PROG_DATA == next_sec->getSectionType() ) {

            output->verbose( CALL_INFO, 2, 0,
                            ">> Loading Section (%" PRIu64 ") from executable at: 0x%0llx, len=%" PRIu64 "...\n",
                            next_sec->getID(), next_sec->getVirtualMemoryStart(), next_sec->getImageLength());

            if ( next_sec->getVirtualMemoryStart() > 0 ) {
                const uint64_t padding = 4096 - ((next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) % 4096);

                // Executable data, let's load it in
                if ( initial_mem_contents.size() < (next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) ) {
                    size_t size_now = initial_mem_contents.size();
                    initial_mem_contents.resize( next_sec->getVirtualMemoryStart() + next_sec->getImageLength() + padding, 0);
                }

                // Find the section and read it all in
                fseek(exec_file, next_sec->getImageOffset(), SEEK_SET); 
                fread( &initial_mem_contents[next_sec->getVirtualMemoryStart()], next_sec->getImageLength(), 1, exec_file);
            } else {
                output->verbose(CALL_INFO, 2, 0, "--> Not loading because virtual address is zero.\n");
            }
        } else if ( SECTION_HEADER_BSS == next_sec->getSectionType() ) {

            output->verbose( CALL_INFO, 2, 0,
                            ">> Loading BSS Section (%" PRIu64 ") with zeroing at 0x%0llx, len=%" PRIu64 "\n",
                            next_sec->getID(), next_sec->getVirtualMemoryStart(), next_sec->getImageLength());

            if ( next_sec->getVirtualMemoryStart() > 0 ) {
                const uint64_t padding = 4096 - ((next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) % 4096);

                // Resize if needed with zeroing
                if ( initial_mem_contents.size() < (next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) ) {
                    size_t size_now = initial_mem_contents.size();
                    initial_mem_contents.resize( next_sec->getVirtualMemoryStart() + next_sec->getImageLength() + padding, 0); 
                }

                // Zero out the section according to the Section header info
                std::memset( &initial_mem_contents[next_sec->getVirtualMemoryStart()], 0, next_sec->getImageLength());
            } else {
                output->verbose(CALL_INFO, 2, 0, "--> Not loading because virtual address is zero.\n");
            }
        } else {
            if ( next_sec->isAllocated() ) {
                output->verbose( CALL_INFO, 2, 0,
                                ">> Loading Allocatable Section (%" PRIu64 ") at 0x%0llx, len: %" PRIu64 "\n",
                                next_sec->getID(), next_sec->getVirtualMemoryStart(), next_sec->getImageLength());

                if ( next_sec->getVirtualMemoryStart() > 0 ) {
                    const uint64_t padding = 4096 - ((next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) % 4096);

                    // Resize if needed with zeroing
                    if ( initial_mem_contents.size() < (next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) ) {
                        size_t size_now = initial_mem_contents.size();
                        initial_mem_contents.resize( next_sec->getVirtualMemoryStart() + next_sec->getImageLength() + padding, 0);
                    }

                    // Find the section and read it all in
                    fseek(exec_file, next_sec->getImageOffset(), SEEK_SET);
                    fread( &initial_mem_contents[next_sec->getVirtualMemoryStart()], next_sec->getImageLength(), 1, exec_file);
               }
           }
        }
    }

    fclose(exec_file);

    output->verbose( CALL_INFO, 2, 0, ">> Writing memory contents (%" PRIu64 " bytes at index 0)\n", (uint64_t)initial_mem_contents.size());

    mem_if->sendUntimedData(new SST::Interfaces::StandardMem::Write( 0, initial_mem_contents.size(), initial_mem_contents ) );

    const uint64_t page_size = 4096;

    uint64_t initial_brk = (uint64_t)initial_mem_contents.size();
    initial_brk          = initial_brk + (page_size - (initial_brk % page_size));

    output->verbose( CALL_INFO, 2, 0,
                    ">> Setting initial break point to image size in "
                    "memory ( brk: 0x%llx )\n",
                    initial_brk);
    return initial_brk;
}


#if 0
    virtual void setInitialMemory(const uint64_t addr, std::vector<uint8_t>& payload) {
        output->verbose(CALL_INFO, 2, 0, "setting initial memory contents for address 0x%llx / size: %" PRIu64 "\n",
                        addr, (uint64_t)payload.size());
        memInterface->sendUntimedData(new StandardMem::Write(addr, payload.size(), payload));

        if (fault_on_memory_not_written) {
            // Mark every byte which we are initializing
            for (uint64_t i = addr; i < (addr + payload.size()); ++i) {
                memory_check_table->markByte(i);
            }
        }
    }

#endif
}
}
