// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <stdint.h>
#include <string>
#include <math.h>
#include "os/velfloader.h"
#include "os/vloadpage.h"

namespace SST {
namespace Vanadis {

static size_t roundDown( size_t num, size_t step ) {
    return num & ~( step - 1 );
}

static size_t roundUp( size_t num, size_t step ) {
    if ( num & ( step - 1 ) ) {
        return roundDown( num, step) + step;
    } else {
        return num;
    }
}

void loadElfFile( Output* output, Interfaces::StandardMem* mem_if, MMU_Lib::MMU* mmu,  PhysMemManager* memMgr, VanadisELFInfo* elf_info, int hwThread,
        int page_size, OS::ProcessInfo* processInfo ) 
{
    // we are not using this for loads for the elf is faulted in, but could use it for a system without virtual memory 
    // so leave it here but fault, it will need to be verified when used
    assert(0);
    uint64_t initial_brk = 0;

    auto path = elf_info->getBinaryPath(); 
    output->verbose( CALL_INFO, 2, 0, "-> Loading %s, to locate program secs ...\n", path);
    FILE* exec_file = fopen(path, "rb");

    if ( nullptr == exec_file ) {
        output->fatal(CALL_INFO, -1, "Error: unable to open %s\n", path );
    }

    output->verbose(CALL_INFO, 2, 0, "-> populating memory contents with info from the executable...\n");

    for ( size_t i = 0; i < elf_info->countProgramHeaders(); ++i ) {

        const VanadisELFProgramHeaderEntry* hdr = elf_info->getProgramHeader(i);
        if ( PROG_HEADER_LOAD == hdr->getHeaderType() ) {
            uint64_t virtAddr = hdr->getVirtualMemoryStart(); 
            size_t   memLen = hdr->getHeaderMemoryLength();
            size_t   imageLen = hdr->getHeaderImageLength();
            uint64_t flags = hdr->getSegmentFlags();

            fseek(exec_file, hdr->getImageOffset(), SEEK_SET); 

            uint64_t virtAddrPage = roundDown( virtAddr, page_size ); 
            uint64_t virtAddrEnd = roundUp(virtAddr + memLen, page_size );
            size_t numBytes = page_size - (virtAddr - virtAddrPage); 
            size_t imageOffset = 0;

            output->verbose( CALL_INFO, 2, 0,"ELF virtAddrStart=%#" PRIx64 " memLen=%zu imageLen=%zu flags=%#" PRIx64 "\n",virtAddr, memLen, imageLen, flags );
            output->verbose( CALL_INFO, 2, 0,"page aligned virtual address region: %#" PRIx64 " - %#" PRIx64 "\n", virtAddrPage, virtAddrEnd );
            std::string name;
            if ( elf_info->getEntryPoint() >= virtAddrPage && elf_info->getEntryPoint() < virtAddrEnd ) {
                name = "text";
            }else{
                name = "data";
            }
            processInfo->addMemRegion( name, virtAddrPage, virtAddrEnd-virtAddrPage, flags );

            while( virtAddrPage < virtAddrEnd ) {
                std::vector<uint8_t> buffer(page_size,0);

                if ( imageOffset < imageLen ) {
                    if ( numBytes > imageLen - imageOffset ) {
                        numBytes = imageLen - imageOffset;
                    }
                    size_t pageOffset = (virtAddr + imageOffset) - virtAddrPage;
                    fread( buffer.data() + pageOffset, numBytes, 1, exec_file);
                    imageOffset += numBytes;
                    output->verbose( CALL_INFO, 2, 0,"write page, pageVirtAddr=%#" PRIx64 " imageOffset=%zu pageOffset=%zu numBytes=%zu\n",
                        virtAddrPage,imageOffset,pageOffset,numBytes); 
                } 

                loadPages( output, mem_if, mmu, memMgr, processInfo->getpid(), virtAddrPage, buffer, flags, page_size );

                virtAddrPage += page_size;
                numBytes = page_size;
            }
            if ( virtAddrEnd > initial_brk ) {
                initial_brk = virtAddrEnd;
            }
        }
    }

    fclose(exec_file);

    output->verbose( CALL_INFO, 2, 0,
                    ">> Setting initial break point to image size in "
                    "memory ( brk: 0x%llx )\n",
                    initial_brk);
    processInfo->initBrk( initial_brk );
}

uint8_t* readElfPage( Output* output, VanadisELFInfo* elf_info, int vpn, int page_size ) {
    uint64_t virtAddr = vpn<<12;  
    auto path = elf_info->getBinaryPath();
    output->verbose( CALL_INFO, 2, 0, "-> Loading %s, to locate program sections ...\n", path);
    output->verbose( CALL_INFO, 2, 0,"%s vpn=%d addr=%#" PRIx64 " page_size=%d\n",path,vpn,virtAddr,page_size);
    FILE* exec_file = fopen(elf_info->getBinaryPath(), "rb");
    if ( nullptr == exec_file ) {
        output->fatal(CALL_INFO, -1, "Error: unable to open %s\n", path);
    }
    uint8_t* data = new uint8_t[page_size];
    bzero(data, page_size); 
    const VanadisELFProgramHeaderEntry* secHdr = elf_info->findProgramHeader( virtAddr );

    assert( secHdr );
    uint64_t secAddr = secHdr->getVirtualMemoryStart(); 
    size_t   secMemLen = secHdr->getHeaderMemoryLength();
    size_t   secImageLen = secHdr->getHeaderImageLength();
    size_t   secImageOffset = secHdr->getImageOffset();
    uint64_t secAlignment = secHdr->getAlignment();
    uint64_t secFlags = secHdr->getSegmentFlags();

    output->verbose( CALL_INFO, 2, 0," section: virtAddr=%#" PRIx64 " imageOffset=%zu memLen=%zu imageLen=%zu flags=%#" PRIx64 " align=%#" PRIx64 "\n",
            secAddr,secImageOffset,secMemLen,secImageLen,secFlags,secAlignment);

    // if the requested address is less than the section virtual address then this is the first page and it's not aligned
    // we read the file from the start of the section
    size_t imageOffset = virtAddr < secAddr ? 0 : virtAddr - secAddr;
     
    // The memory footprint of the section can be greater than the file image.
    // if the imageOffset is less than the secImageLen we had data to transfer
    if ( imageOffset < secImageLen ) {
        // we write the data to offset in the buffer 
        size_t dataOffset = virtAddr < secAddr ? secAddr - virtAddr : 0;

        // at most we read a page
        size_t numBytes = page_size;
    
        // if we are not aligned we can, at most, read from the unaligned start to the end of the page
        numBytes -= dataOffset;

        // if there is not enough data to read a full pagge
        numBytes = secImageLen - imageOffset < numBytes ? secImageLen - imageOffset : numBytes;
    
        output->verbose( CALL_INFO, 2, 0,"imageOffset=%zu dataOffset=%zu numBytes=%zu toEnd=%zu\n", imageOffset, dataOffset, numBytes, secImageLen - imageOffset );
        fseek(exec_file, secImageOffset + imageOffset, SEEK_SET); 

        fread( data + dataOffset, numBytes, 1, exec_file);
    }

    fclose(exec_file);
    return data; 
}


} // namespace Vanadis
} // namespace SST 
