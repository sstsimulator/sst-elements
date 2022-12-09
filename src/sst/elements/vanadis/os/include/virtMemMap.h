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

#ifndef _H_VANADIS_NODE_OS_INCLUDE_VIRT_MEM_MAP
#define _H_VANADIS_NODE_OS_INCLUDE_VIRT_MEM_MAP

#include <queue>
#include <iterator>
#include <string>
#include <string.h>
#include "velf/velfinfo.h"
#include "os/include/freeList.h"
#include "os/include/page.h"

#if 0
#define VirtMemDbg( format, ... ) printf( "VirtMemMap::%s() " format, __func__, ##__VA_ARGS__ )
#else
#define VirtMemDbg( format, ... )
#endif

#if 0
#define MemoryRegionDbg( format, ... ) printf( "MemoryRegion::%s() " format, __func__, ##__VA_ARGS__ )
#else
#define MemoryRegionDbg( format, ... )
#endif


namespace SST {
namespace Vanadis {

namespace OS {

struct MemoryBacking {
    MemoryBacking( VanadisELFInfo* elfInfo = nullptr ) : elfInfo( elfInfo ) {}
    VanadisELFInfo* elfInfo;
    std::vector<uint8_t> data;
    uint64_t dataStartAddr;
};

struct MemoryRegion {

    MemoryRegion() : name(""), addr(0), length(0), perms(0)  {}
    MemoryRegion( std::string name, uint64_t addr, size_t length, uint32_t perms, MemoryBacking* backing = nullptr)
        : name(name),addr(addr),length(length),perms(perms),backing(backing) {
        MemoryRegionDbg("%s [%#" PRIx64  " - %#" PRIx64 "] backing=%p\n",name.c_str(), addr, addr+length, backing );
    }

    ~MemoryRegion() {
        for ( auto kv: m_virtToPhysMap) { 
            auto page = kv.second;
            if ( 0 == page->decRefCnt() ) {
                delete page;
            }
        }
    }
    void incPageRefCnt() {
        for ( auto kv: m_virtToPhysMap) { 
            kv.second->incRefCnt();
        }
    }

    void mapVirtToPhys( unsigned vpn, OS::Page* page ) {
        OS::Page* ret = nullptr;
        MemoryRegionDbg("vpn=%d ppn=%d refCnt=%d\n", vpn, page->getPPN(),page->getRefCnt());
        if( m_virtToPhysMap.find(vpn) != m_virtToPhysMap.end() ) {
            auto* tmp = m_virtToPhysMap[vpn];
            MemoryRegionDbg("decRef ppn=%d refCnt=%d\n", tmp->getPPN(),tmp->getRefCnt()-1);
            if ( 0 == tmp->decRefCnt() ) {
                delete tmp;
            }
        }
        m_virtToPhysMap[ vpn ] = page;
    }
    uint64_t end() { return addr + length; }

    std::string name;
    uint64_t addr;
    size_t length;
    uint32_t perms;

    uint8_t* readData( uint64_t pageAddr, int pageSize ) {
        uint8_t* data = new uint8_t[pageSize];
        size_t offset = pageAddr - addr;
        MemoryRegionDbg("region start %#" PRIx64 ", page addr %#" PRIx64 ", dataStartAddr %#" PRIx64 "\n",addr, pageAddr, backing->dataStartAddr);
        if ( pageAddr >= backing->dataStartAddr && pageAddr + pageSize <= backing->dataStartAddr + backing->data.size() ) {

            size_t offset = pageAddr - backing->dataStartAddr;
            MemoryRegionDbg("copy data"); 
            memcpy( data, backing->data.data() + offset, pageSize );
        } else {
            MemoryRegionDbg("zero data"); 
            bzero( data, pageSize );
        }
        return data;
    }

    MemoryBacking* backing; 

  private:
    std::map<unsigned, OS::Page* > m_virtToPhysMap;
};


class VirtMemMap {

public:
    VirtMemMap() : m_refCnt(1) { 
        m_freeList = new FreeList( 0x1000, 0x80000000); 
    }

    VirtMemMap( const VirtMemMap& obj ) : m_refCnt(1) {
        for ( const auto& kv: obj.m_regionMap) {
            m_regionMap[kv.first] = new MemoryRegion( *kv.second );
            if ( kv.second == obj.m_heapRegion ) {
                m_heapRegion = m_regionMap[kv.first];
            }
            kv.second->incPageRefCnt();
        }
        m_freeList = new FreeList( *obj.m_freeList );
    }

    ~VirtMemMap() {
        for (auto iter = m_regionMap.begin(); iter != m_regionMap.end(); iter++) {
            delete iter->second;
        }
    
        delete m_freeList;
    }

    int refCnt() { return m_refCnt; }
    void decRefCnt() { assert( m_refCnt > 0 ); --m_refCnt; };
    void incRefCnt() { ++m_refCnt; };

    uint64_t addRegion( std::string name, uint64_t start, size_t length, uint32_t perms, MemoryBacking* backing = nullptr ) {
        VirtMemDbg("%s addr=%#" PRIx64 " length=%zu perms=%#x\n", name.c_str(), start, length, perms );

        if ( start ) {
            assert( m_freeList->alloc( start, length ) );
        } else {
            assert( start = m_freeList->alloc( length ) );
        }
        
        m_regionMap[start] = new MemoryRegion( name, start, length, perms, backing ); 
       
        print(name); 
        return start;
    }

    void rmRegion( MemoryRegion* region ) {

        VirtMemDbg("name=`%s` [%#" PRIx64 " - %#" PRIx64 "]\n",region->name.c_str(),region->addr,region->addr+region->length);

        m_freeList->free( region->addr, region->length );

        m_regionMap.erase( region->addr );

        delete region;
    }

    MemoryRegion* findRegion( uint64_t addr ) {
        VirtMemDbg("addr=%#" PRIx64 "\n", addr );
        auto iter = m_regionMap.begin();
        for ( ; iter != m_regionMap.end(); ++iter ) {
            auto region = iter->second; 
            VirtMemDbg("region %s [%#" PRIx64 " - %#" PRIx64 "] length=%zu perms=%#x\n",
                    region->name.c_str(),region->addr,region->addr+region->length, region->length,region->perms);
            if ( addr >= region->addr && addr < (unsigned) (region->addr + region->length ) ) {
                return region;
            }
        } 
        return nullptr;
    }

    void mprotect( uint64_t addr, size_t length, int prot ) {
        auto* region = findRegion( addr );
        if ( addr == region->addr ) {
            if ( length < region->length ) {
                region->addr = addr + length;
                region->length -= length;
                m_regionMap[addr] = new MemoryRegion( "", addr, length, prot ); 
            } else if ( length == region->length ) {
                region->perms = prot;
            } else {
                // we currently are only supporting splitting a region
                assert(0);
            }
        } else if ( ( addr < region->addr + region->length ) && ( addr + length == region->end() ) ) {
            region->length -= length; 
            m_regionMap[addr] = new MemoryRegion( "", addr, length, prot ); 
        } else {
                // we currently are only supporting splitting a region
                assert(0);
        }
    }

    void print(std::string msg) {

#ifdef VIRT_MEM_DBG 
        auto iter = m_regionMap.begin();
        printf("Process VM regions: %s\n",msg.c_str());
        for ( ; iter != m_regionMap.end(); ++iter ) {
            auto region = iter->second; 
            std::string perms; 
            if ( region->perms & 1<<2 ) {
                perms += "r"; 
            } else {
                perms += "-"; 
            }
            if ( region->perms & 1<<1 ) {
                perms += "w"; 
            } else {
                perms += "-"; 
            }
            if ( region->perms & 1<<0 ) {
                perms += "x"; 
            } else {
                perms += "-"; 
            }
            printf("%#" PRIx64 "-%#" PRIx64 " %s [%s]\n",region->addr, region->addr + region->length, perms.c_str(), region->name.c_str());
        }
        printf("\n");
#endif
    }

    uint64_t mmap( size_t length, uint32_t perms ) {
        uint64_t start = addRegion( "", 0, length, perms );    
        VirtMemDbg("lenght=%zu perms=%#x start=%#" PRIx64 "\n",length,perms,start);
        return start;
    }

    int unmap( uint64_t addr, size_t length ) {
        VirtMemDbg("addr=%#" PRIx64 " length=%zu\n", addr, length);

        std::deque<MemoryRegion*> regions;
        do {
            auto* region = findRegion(addr); 
            if ( nullptr == region ) return EINVAL;

            VirtMemDbg("found region %s() addr=%#" PRIx64 " length=%zu\n",region->name.c_str(), region->addr, region->length);

            if ( addr == region->addr && length > region->length || length == region->length ) {
                regions.push_back(region);
                addr += region->length;
                length -= region->length;
            } else {
                // we are not currently supporting unmapping part of region
                assert(0);
            }

        } while ( length );

        for ( const auto tmp : regions ) {
            VirtMemDbg("delete region %s() addr=%#" PRIx64 " length=%zu\n",tmp->name.c_str(), tmp->addr, tmp->length);
            rmRegion( tmp );
        }
        print("unmap done");
        return 0;
    }

    void initBrk( uint64_t addr ) { 
        VirtMemDbg("brk=%#" PRIx64 "\n",addr);
        m_regionMap[addr] = m_heapRegion = new MemoryRegion( "heap", addr, 0, 0x6, NULL ); 
    }

    uint64_t getBrk() { 
        VirtMemDbg("brk=%#" PRIx64 "\n",m_heapRegion->addr + m_heapRegion->length);
        return m_heapRegion->addr + m_heapRegion->length;  
    }

    bool setBrk( uint64_t brk ) { 
        size_t length = brk - m_heapRegion->addr;  
        bool ret = m_freeList->update( m_heapRegion->addr, length );

        VirtMemDbg("brk=%#" PRIx64 "\n",m_heapRegion->addr + m_heapRegion->length);

        if ( ret ) {
            m_heapRegion->length = length;  
        }
        
        return ret;
    }

private:

    std::map< uint64_t, MemoryRegion* > m_regionMap;

    MemoryRegion*   m_heapRegion;
    FreeList*       m_freeList;
    int             m_refCnt;
};

}
}
}

#endif
