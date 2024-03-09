// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
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

#if 0
#define VirtMemDbg( format, ... ) printf( "VirtMemMap::%s() " format, __func__, ##__VA_ARGS__ )
#define VIRT_MEM_DBG 1
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

class MemoryBacking {
public:
    MemoryBacking( VanadisELFInfo* elfInfo = nullptr ) : elfInfo( elfInfo ), dev(nullptr), dataStartAddr(0) {}
    MemoryBacking( Device* dev ) : elfInfo( nullptr ), dev(dev), dataStartAddr(0) {}
    VanadisELFInfo* elfInfo;
    Device* dev;
    std::vector<uint8_t> data;
    uint64_t dataStartAddr;

    MemoryBacking( SST::Output* output, FILE* fp, VanadisELFInfo* elf ) {
        char* tmp = nullptr;
        size_t num = 0;
        getline( &tmp, &num, fp );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s",tmp);
        assert( 0 == strcmp(tmp,"#MemoryBacking start\n") );
        free(tmp);

        char str[80];
        assert( 1 == fscanf(fp,"backing: %s\n",str) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"backing: %s\n",str);

        if ( 0 == strcmp( str, "elf" ) ) {
            assert ( 1 == fscanf(fp,"elf: %s\n", str ) );
            output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"elf: %s\n",str);
            assert( 0 == strcmp( str, elf->getBinaryPath() ) );
            elfInfo = elf;
        } else if ( 0 == strcmp( str, "data" ) ) {
            assert ( 1 == fscanf(fp,"dataStartAddr: %" PRIx64 "\n",&dataStartAddr) ); 
            output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"dataStartAddr: %#" PRIx64 "\n",dataStartAddr );
            size_t size;
            assert( 1 == fscanf(fp,"size: %zu\n", &size ) );
            output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"size: %zu\n", size );
            data.resize(size);
            std::stringstream ss;
            ss << std::hex;
            for ( auto i = 0; i < size; i++ ) {
                uint64_t value;
                assert( 1 == fscanf(fp,"%" PRIx64 ",", &value) );
                data[i] = value;
                ss << "0x" << value << ",";
            }
            fscanf(fp,"\n" );
            output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s\n",ss.str().c_str());

        } else {
            assert(0);
        }

        tmp = nullptr;
        num = 0;
        getline( &tmp, &num, fp );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s",tmp);
        assert( 0 == strcmp(tmp,"#MemoryBacking end\n") );
        free(tmp);
    }

    void checkpoint( FILE* fp ) {
        fprintf(fp,"#MemoryBacking start\n");
        if ( elfInfo ) {
            fprintf(fp,"backing: elf\n");
            fprintf(fp,"elf: %s\n",elfInfo->getBinaryPath());
        } else if ( data.size() ) {
            fprintf(fp,"backing: data\n");
            fprintf(fp,"dataStartAddr: %#" PRIx64 "\n",dataStartAddr);
            auto ptr = (uint64_t*) data.data();
            fprintf(fp,"size: %zu\n", data.size()/sizeof(uint64_t) );
            for ( auto i = 0; i < data.size()/sizeof(uint64_t); i++ ) {
                fprintf(fp,"%#" PRIx64 ",",ptr[i]);
            }
            fprintf(fp,"\n");
        } else {
            assert(0);
        }
        fprintf(fp,"#MemoryBacking end\n");
    }
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
            // don't delete text pages because they are in the page cache
            if ( name.compare("text" ) ) {
                if ( 0 == page->decRefCnt() ) {
                    delete page;
                }
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
    
    void checkpoint( FILE* fp ) {
        fprintf(fp,"#MemoryRegion start\n");
        fprintf(fp,"name: %s\n",name.c_str());
        fprintf(fp,"addr: %#" PRIx64 "\n",addr);
        fprintf(fp,"length: %zu\n",length);
        fprintf(fp,"perms: %#" PRIx32 "\n",perms);
        if ( backing ) {
            fprintf(fp,"backing: yes\n");
            backing->checkpoint( fp );
        } else {
            fprintf(fp,"backing: no\n");
        }

        fprintf(fp,"m_virtToPhysMap.size() %zu\n",m_virtToPhysMap.size());
        for ( auto & x : m_virtToPhysMap ) {
            fprintf(fp,"vpn: %d, %s\n", x.first, x.second->checkpoint().c_str() );
        }
        fprintf(fp,"#MemoryRegion end\n");
    }

    MemoryRegion( SST::Output* output, FILE* fp, PhysMemManager* memManager, VanadisELFInfo* elfInfo ) : backing(nullptr) {
        char* tmp = nullptr;
        size_t num = 0;
        getline( &tmp, &num, fp );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s",tmp);
        assert( 0 == strcmp(tmp,"#MemoryRegion start\n"));
        free(tmp);

        char str[80];
        assert( 1 == fscanf(fp,"name: %s\n",str));
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"name: %s\n",str);
        name = str; 

        assert( 1 == fscanf(fp,"addr: %" PRIx64 "\n",&addr) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"addr: %#" PRIx64 "\n",addr);

        assert( 1 == fscanf(fp,"length: %zu\n",&length) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"length: %zu\n",length);

        assert( 1 == fscanf(fp,"perms: %" PRIx32 "\n",&perms) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"perms: %#" PRIx32 "\n",perms);

        assert( 1 == fscanf(fp,"backing: %s\n", str) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"backing: %s\n", str );
        if ( 0 == strcmp( str, "yes" ) ) {
            backing = new MemoryBacking( output, fp, elfInfo );
        }

        size_t size;
        assert( 1 == fscanf(fp,"m_virtToPhysMap.size() %zu\n",&size) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_virtToPhysMap.size() %zu\n",size);

        for ( auto i = 0; i < size; i++ ) {
            int vpn,ppn,refCnt;
            assert( 3 == fscanf(fp,"vpn: %d, ppn: %d, refCnt: %d\n", &vpn, &ppn, &refCnt ) );
            output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"vpn: %d, ppn: %d, refCnt: %d\n", vpn, ppn, refCnt );
            m_virtToPhysMap[vpn] = new OS::Page( memManager, ppn, refCnt );
        }

        tmp = nullptr;
        num = 0;
        getline( &tmp, &num, fp );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s",tmp);
        assert( 0 == strcmp(tmp,"#MemoryRegion end\n"));
        free(tmp);
    }

    MemoryBacking* backing; 
    OS::Page* getPage( int vpn ) {
        auto iter = m_virtToPhysMap.find( vpn );
        assert( iter != m_virtToPhysMap.end() );
        return iter->second;
    }

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

    MemoryRegion* findRegion( std::string name ) {
        VirtMemDbg("name=%s\n", name.c_str() );
        auto iter = m_regionMap.begin();
        for ( ; iter != m_regionMap.end(); ++iter ) {
            auto region = iter->second; 
            VirtMemDbg("region %s [%#" PRIx64 " - %#" PRIx64 "] length=%zu perms=%#x\n",
                    region->name.c_str(),region->addr,region->addr+region->length, region->length,region->perms);
            if ( 0 == strcmp( name.c_str(), region->name.c_str() ) ) { 
                return region;
            }
        } 
        return nullptr;
    }

    void mprotect( uint64_t addr, size_t length, int prot ) {
        VirtMemDbg("addr=%#" PRIx64 " length=%zu prot=%#x\n", addr, length, prot );
        auto* region = findRegion( addr );
        if ( addr == region->addr ) {
            // split region in two 
            if ( length < region->length ) {
                // create part two of split
                m_regionMap[addr+length] = new MemoryRegion( region->name, addr + length, region->length - length, region->perms ); 
                // update part one of split
                region->length = length;
                region->perms = prot;
            // mprotect complete region
            } else if ( length == region->length ) {
                region->perms = prot;
            } else {
                // we currently are only supporting splitting a region
                assert(0);
            }
        } else if ( ( addr < region->addr + region->length ) && ( addr + length == region->end() ) ) {
            region->length -= length; 
            m_regionMap[addr] = new MemoryRegion( region->name, addr, length, prot ); 
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

    uint64_t mmap( size_t length, uint32_t perms, Device* dev ) {
        std::string name("mmap");
        MemoryBacking* backing = nullptr;
        if ( dev ) {
            backing = new MemoryBacking( dev );
            name = dev->getName();
        }

        uint64_t start = addRegion( name, 0, length, perms, backing );
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
        auto start = addRegion( "heap", addr, 0x10000000, 0x6 );
        assert( addr == start );
        m_heapRegion = m_regionMap[addr];
        m_brk = addr;
    }

    uint64_t getBrk() { 
        return m_brk;
    }

    bool setBrk( uint64_t brk ) { 
        assert( brk >= m_brk );
        assert( brk < m_heapRegion->addr + m_heapRegion->length );
        m_brk = brk;
        return true;
    }

    void checkpoint( FILE* fp ) {
        fprintf(fp,"#VirtMemMap start\n");
        fprintf(fp,"m_brk: %#" PRIx64 "\n",m_brk);    
        fprintf(fp,"m_refCnt: %d\n",m_refCnt);    
        fprintf(fp,"m_regionMap.size() %zu\n",m_regionMap.size());    

        for ( auto & x : m_regionMap ) {
            fprintf(fp,"addr: %#" PRIx64 "\n",x.first);    
            x.second->checkpoint(fp);
        }
        fprintf(fp,"#VirtMemMap end\n");
    }

    VirtMemMap( SST::Output* output, FILE* fp, PhysMemManager* memManager, VanadisELFInfo* elfInfo) {
        char* str = nullptr;
        size_t num = 0;
        getline( &str, &num, fp );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s",str);
        assert( 0 == strcmp(str,"#VirtMemMap start\n"));
        free(str);

        uint64_t foo;
        assert( 1 == fscanf(fp,"m_brk: %" PRIx64 "\n",&foo));    
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_brk: %#" PRIx64"\n",foo);    

        assert( 1 == fscanf(fp,"m_refCnt: %d\n",&m_refCnt));    
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_refCnt: %d\n",m_refCnt);    

        size_t size;
        assert( 1 == fscanf(fp,"m_regionMap.size() %zu\n",&size));    
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_regionMap.size() %zu\n",size);    
        for ( auto i = 0; i < size; i++ ) {
            uint64_t addr;
            assert( 1 == fscanf(fp,"addr: %" PRIx64 "\n",&addr));    
            output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"addr: %#" PRIx64 "\n",addr);    

            m_regionMap[addr] = new MemoryRegion( output, fp, memManager, elfInfo );
        }

        str = nullptr;
        num = 0;
        getline( &str, &num, fp );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s",str);
        assert( 0 == strcmp(str,"#VirtMemMap end\n"));
        free(str);
    }

private:

    std::map< uint64_t, MemoryRegion* > m_regionMap;

    uint64_t        m_brk;
    MemoryRegion*   m_heapRegion;
    FreeList*       m_freeList;
    int             m_refCnt;
};

}
}
}

#endif
