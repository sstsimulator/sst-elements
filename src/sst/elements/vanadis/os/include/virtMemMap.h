// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_NODE_OS_INCLUDE_VIRT_MEM_MAP
#define _H_VANADIS_NODE_OS_INCLUDE_VIRT_MEM_MAP

#include <queue>
#include <iterator>
#include <string>
#include <string.h>
#include "velf/velfinfo.h"
#include "os/include/freeList.h"
#include "os/include/page.h"
#include "os/include/device.h"


namespace SST {
namespace Vanadis {

namespace OS {

/** MemoryBacking: holds information about memory contents for initializing non-zero regions of memory
 * that may not have been paged into simulated memory (e.g., program text, data, etc.)
 */
class MemoryBacking {
public:
    // Backing linked to elf info
    MemoryBacking( VanadisELFInfo* elfInfo = nullptr );

    // Backing linked to mmap region
    MemoryBacking( Device* dev );

    // Backing linked to vanadis application snapshot
    MemoryBacking( SST::Output* output, FILE* fp, VanadisELFInfo* elf );

    // Snapshot the backing
    void snapshot( FILE* fp );

    VanadisELFInfo* elf_info_;
    Device* dev_;
    std::vector<uint8_t> data_;
    uint64_t data_start_addr_;
};


/** A segment of the process's virtual memory
 * Name: text, data, heap, stack, etc.
 * Perms: permissions for this region
 * Addr/length: define the region
 * Backing:
 *  If segment is linked to a file or application data (e.g., text/data segements and mmap regions),
 *     backing will point to the data
 *      IMPORTANT: Multiple regions may point to the same backing instance
 *  Otherwise (e.g., for heap), backing is nullptr
 */
class MemoryRegion {
public:
    // Default constructor
    MemoryRegion();

    // Construct a region of type 'name'
    MemoryRegion( std::string name, uint64_t addr, size_t length, uint32_t perms, MemoryBacking* backing = nullptr);
    MemoryRegion( SST::Output* output, FILE* fp, PhysMemManager* memManager, VanadisELFInfo* elfInfo );

    ~MemoryRegion();

    void incPageRefCnt();

    void mapVirtToPhys( unsigned vpn, OS::Page* page );

    uint64_t end();

    uint8_t* readData( uint64_t pageAddr, int pageSize );

    void snapshot( FILE* fp );

    OS::Page* getPage( int vpn );

    std::string name_;
    uint64_t addr_;
    size_t length_;
    uint32_t perms_;
    MemoryBacking* backing_;

  private:
    std::map<unsigned, OS::Page* > virt_to_phys_page_map_;
};


/** Class holds a map of a process's virtual address regions */
class VirtMemMap {

public:
    VirtMemMap();

    VirtMemMap( const VirtMemMap& obj );

    ~VirtMemMap();

    int refCnt();
    void decRefCnt();
    void incRefCnt();

    uint64_t addRegion( std::string name, uint64_t start, size_t length, uint32_t perms, MemoryBacking* backing = nullptr );

    void removeRegion( MemoryRegion* region );

    void shrinkRegion( MemoryRegion* region, uint64_t start, size_t length );

    MemoryRegion* findRegion( uint64_t addr );
    MemoryRegion* findRegion( std::string name );

    void mprotect( uint64_t addr, size_t length, int prot );

    void print(std::string msg);

    uint64_t mmap( size_t length, uint32_t perms, Device* dev );

    int unmap( uint64_t addr, size_t length );

    void initBrk( uint64_t addr );

    uint64_t getBrk();

    bool setBrk( uint64_t brk );

    void snapshot( FILE* fp );

    VirtMemMap( SST::Output* output, FILE* fp, PhysMemManager* memManager, VanadisELFInfo* elfInfo);

private:
    std::map< uint64_t, MemoryRegion* > region_map_; // Map of start address to region
    uint64_t        brk_;
    MemoryRegion*   heap_region_;
    FreeList*       free_list_;
    int             ref_cnt_;
};

}
}
}

#endif
