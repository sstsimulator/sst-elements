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

#include <sst_config.h>
#include "os/include/virtMemMap.h"

#include <queue>
#include <iterator>
#include <string>
#include <string.h>
#include "velf/velfinfo.h"
#include "os/include/freeList.h"
#include "os/include/page.h"
#include "os/include/device.h"

// This is temporary debug support while we stabilize vanadis
// Will be removed/replaced with proper SST output eventually
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

using namespace SST::Vanadis::OS;

/** Memory Backing */

// Backing linked to app
MemoryBacking::MemoryBacking( VanadisELFInfo* elf_info ) :
        elf_info_( elf_info ), dev_(nullptr), data_start_addr_(0)
{}

// Backing linked to mmap
MemoryBacking::MemoryBacking( Device* dev ) : elf_info_( nullptr ), dev_( dev ), data_start_addr_( 0 )
{}

// Backing linked to application snapshot
MemoryBacking::MemoryBacking( SST::Output* output, FILE* fp, VanadisELFInfo* elf )
{
    char* tmp = nullptr;
    size_t num = 0;
    (void) !getline( &tmp, &num, fp );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"%s",tmp);
    assert( 0 == strcmp(tmp,"#MemoryBacking start\n") );
    free(tmp);

    char str[80];
    assert( 1 == fscanf(fp,"backing: %s\n",str) );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"backing: %s\n",str);

    if ( 0 == strcmp( str, "elf" ) ) {
        assert ( 1 == fscanf(fp,"elf: %s\n", str ) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"elf: %s\n",str);
        assert( 0 == strcmp( str, elf->getBinaryPath() ) );
        elf_info_ = elf;
    } else if ( 0 == strcmp( str, "data" ) ) {
        assert ( 1 == fscanf(fp,"dataStartAddr: %" PRIx64 "\n",&data_start_addr_) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"dataStartAddr: %#" PRIx64 "\n",data_start_addr_ );
        size_t size;
        assert( 1 == fscanf(fp,"size: %zu\n", &size ) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"size: %zu\n", size );
        data_.resize(size);
        std::stringstream ss;
        ss << std::hex;
        for ( auto i = 0; i < size; i++ ) {
            uint64_t value;
            assert( 1 == fscanf(fp,"%" PRIx64 ",", &value) );
            data_[i] = value;
            ss << "0x" << value << ",";
        }
        (void) !fscanf(fp,"\n" );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"%s\n",ss.str().c_str());
    } else {
        assert(0);
    }

    tmp = nullptr;
    num = 0;
    (void) !getline( &tmp, &num, fp );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"%s",tmp);
    assert( 0 == strcmp(tmp,"#MemoryBacking end\n") );
    free(tmp);
}

void MemoryBacking::snapshot( FILE* fp )
{
    fprintf(fp,"#MemoryBacking start\n");
    if ( elf_info_ ) {
        fprintf(fp,"backing: elf\n");
        fprintf(fp,"elf: %s\n",elf_info_->getBinaryPath());
    } else if ( data_.size() ) {
        fprintf(fp,"backing: data\n");
        fprintf(fp,"dataStartAddr: %#" PRIx64 "\n",data_start_addr_);
        auto ptr = (uint64_t*) data_.data();
        fprintf(fp,"size: %zu\n", data_.size()/sizeof(uint64_t) );
        for ( auto i = 0; i < data_.size()/sizeof(uint64_t); i++ ) {
            fprintf(fp,"%#" PRIx64 ",",ptr[i]);
        }
        fprintf(fp,"\n");
    } else {
        assert(0);
    }
    fprintf(fp,"#MemoryBacking end\n");
}



/** Memory Region */

MemoryRegion::MemoryRegion() : name_(""), addr_(0), length_(0), perms_(0)
{}

MemoryRegion::MemoryRegion( std::string name, uint64_t addr, size_t length, uint32_t perms, MemoryBacking* backing )
    : name_(name), addr_(addr), length_(length), perms_(perms), backing_(backing)
{
    MemoryRegionDbg("%s [%#" PRIx64  " - %#" PRIx64 "]\n",name_.c_str(), addr_, addr_+length_);
}

MemoryRegion::~MemoryRegion()
{
    // don't delete text pages because they are in the page cache
    if ( name_.compare("text" ) ) {
        for ( auto kv: virt_to_phys_page_map_) {
            auto page = kv.second;
            if ( 0 == page->decRefCnt() ) {
                delete page;
            }
        }
    }
}

void MemoryRegion::incPageRefCnt()
{
    for ( auto kv: virt_to_phys_page_map_) {
        kv.second->incRefCnt();
        }
}

void MemoryRegion::mapVirtToPhys( unsigned vpn, OS::Page* page )
{
    OS::Page* ret = nullptr;
    MemoryRegionDbg("vpn=%d ppn=%d refCnt=%d\n", vpn, page->getPPN(),page->getRefCnt());
    if( virt_to_phys_page_map_.find(vpn) != virt_to_phys_page_map_.end() ) {
        auto* tmp = virt_to_phys_page_map_[vpn];
        MemoryRegionDbg("decRef ppn=%d refCnt=%d\n", tmp->getPPN(),tmp->getRefCnt()-1);
        if ( 0 == tmp->decRefCnt() ) {
            delete tmp;
        }
    }
    virt_to_phys_page_map_[ vpn ] = page;
}

uint64_t MemoryRegion::end()
{
    return addr_ + length_;
}

uint8_t* MemoryRegion::readData( uint64_t page_addr, int page_size )
{
    uint8_t* data = new uint8_t[page_size];
    size_t offset = page_addr - addr_;
    MemoryRegionDbg("region start %#" PRIx64 ", page addr %#" PRIx64 ", dataStartAddr %#" PRIx64 "\n",addr_, page_addr, backing_->data_start_addr_);
    if ( page_addr >= backing_->data_start_addr_
        && page_addr + page_size <= backing_->data_start_addr_ + backing_->data_.size() )
    {
        size_t offset = page_addr - backing_->data_start_addr_;
        MemoryRegionDbg("copy data");
        memcpy( data, backing_->data_.data() + offset, page_size );
    } else {
        MemoryRegionDbg("zero data");
        bzero( data, page_size );
    }
    return data;
}

void MemoryRegion::snapshot( FILE* fp )
{
    fprintf(fp,"#MemoryRegion start\n");
    fprintf(fp,"name: %s\n",name_.c_str());
    fprintf(fp,"addr: %#" PRIx64 "\n",addr_);
    fprintf(fp,"length: %zu\n",length_);
    fprintf(fp,"perms: %#" PRIx32 "\n",perms_);
    if ( backing_ ) {
        fprintf(fp,"backing: yes\n");
        backing_->snapshot( fp );
    } else {
        fprintf(fp,"backing: no\n");
    }

    fprintf(fp,"virt_to_phys_page_map_.size() %zu\n",virt_to_phys_page_map_.size());
    for ( auto & x : virt_to_phys_page_map_ ) {
        fprintf(fp,"vpn: %d, %s\n", x.first, x.second->snapshot().c_str() );
    }
    fprintf(fp,"#MemoryRegion end\n");
}

MemoryRegion::MemoryRegion( SST::Output* output, FILE* fp, PhysMemManager* mem_manager, VanadisELFInfo* elf_info ) :
        backing_(nullptr)
{
    char* tmp = nullptr;
    size_t num = 0;
    (void) !getline( &tmp, &num, fp );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"%s",tmp);
    assert( 0 == strcmp(tmp,"#MemoryRegion start\n"));
    free(tmp);

    char str[80];
    assert( 1 == fscanf(fp,"name: %s\n",str));
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"name: %s\n",str);
    name_ = str;

    assert( 1 == fscanf(fp,"addr: %" PRIx64 "\n",&addr_) );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"addr: %#" PRIx64 "\n",addr_);

    assert( 1 == fscanf(fp,"length: %zu\n",&length_) );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"length: %zu\n",length_);

    assert( 1 == fscanf(fp,"perms: %" PRIx32 "\n",&perms_) );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"perms: %#" PRIx32 "\n",perms_);

    assert( 1 == fscanf(fp,"backing: %s\n", str) );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"backing: %s\n", str );
    if ( 0 == strcmp( str, "yes" ) ) {
        backing_ = new MemoryBacking( output, fp, elf_info );
    }

    size_t size;
    assert( 1 == fscanf(fp,"virt_to_phys_page_map_.size() %zu\n",&size) );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"virt_to_phys_page_map_.size() %zu\n",size);

    for ( auto i = 0; i < size; i++ ) {
        int vpn,ppn,ref_cnt;
        assert( 3 == fscanf(fp,"vpn: %d, ppn: %d, refCnt: %d\n", &vpn, &ppn, &ref_cnt ) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"vpn: %d, ppn: %d, refCnt: %d\n", vpn, ppn, ref_cnt );
        virt_to_phys_page_map_[vpn] = new OS::Page( mem_manager, ppn, ref_cnt );
    }

    tmp = nullptr;
    num = 0;
    (void) !getline( &tmp, &num, fp );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"%s",tmp);
    assert( 0 == strcmp(tmp,"#MemoryRegion end\n"));
    free(tmp);
}

Page* MemoryRegion::getPage( int vpn )
{
    auto iter = virt_to_phys_page_map_.find( vpn );
    assert( iter != virt_to_phys_page_map_.end() );
    return iter->second;
}



/** Virtual Memory Map */

VirtMemMap::VirtMemMap() : ref_cnt_(1)
{
    free_list_ = new FreeList( 0x1000, 0x80000000);
}

VirtMemMap::VirtMemMap( const VirtMemMap& obj ) : ref_cnt_(1) {
    for ( const auto& kv: obj.region_map_) {
        region_map_[kv.first] = new MemoryRegion( *kv.second );
        if ( kv.second == obj.heap_region_ ) {
            heap_region_ = region_map_[kv.first];
        }
        kv.second->incPageRefCnt();
    }
    free_list_ = new FreeList( *obj.free_list_ );
}

VirtMemMap::~VirtMemMap() {
    for (auto iter = region_map_.begin(); iter != region_map_.end(); iter++) {
        delete iter->second;
    }
    delete free_list_;
}

int VirtMemMap::refCnt() { return ref_cnt_; }
void VirtMemMap::decRefCnt() { assert( ref_cnt_ > 0 ); --ref_cnt_; };
void VirtMemMap::incRefCnt() { ++ref_cnt_; };

uint64_t VirtMemMap::addRegion(
    std::string name, uint64_t start, size_t length, uint32_t perms, MemoryBacking* backing )
{
    VirtMemDbg("%s addr=%#" PRIx64 " length=%zu perms=%#x\n", name.c_str(), start, length, perms );
    std::fflush(stdout);

    if ( start ) {
        assert( free_list_->alloc( start, length ) );
    } else {
        assert( start = free_list_->alloc( length ) );
    }

    region_map_[start] = new MemoryRegion( name, start, length, perms, backing );

    print(name);
    return start;
}

void VirtMemMap::removeRegion( MemoryRegion* region )
{
    VirtMemDbg("name=`%s` [%#" PRIx64 " - %#" PRIx64 "]\n",
        region->name_.c_str(),region->addr_,region->addr_+region->length_);

    free_list_->free( region->addr_, region->length_ );

    region_map_.erase( region->addr_ );

    delete region;
}

void VirtMemMap::shrinkRegion( MemoryRegion* region, uint64_t addr, size_t length )
{
    VirtMemDbg("%s addr=%#" PRIx64 "->%#" PRIx64 " length=%zu->%zu\n",
        region->name_.c_str(), region->addr_, addr, region->length_, length);

    uint64_t free_addr = (addr == region->addr_) ? addr + length : addr;
    uint64_t free_length = region->length_ - length; // shrinking so length < region->length_

    free_list_->free( free_addr, free_length );

    // Move region to a new start addr
    if ( addr != region->addr_ ) {
        region_map_.erase ( region->addr_ );
        region_map_.insert( std::make_pair(addr, region) );
        region->addr_ = addr;
    }
    // Adjust region length
    region->length_ = length;
}

MemoryRegion* VirtMemMap::findRegion( uint64_t addr )
{
    VirtMemDbg("addr=%#" PRIx64 "\n", addr );
    auto iter = region_map_.begin();
    for ( ; iter != region_map_.end(); ++iter ) {
        auto region = iter->second;
        VirtMemDbg("region %s [%#" PRIx64 " - %#" PRIx64 "] length=%zu perms=%#x, backing=%s\n",
                region->name_.c_str(),region->addr_,region->addr_+region->length_, region->length_,region->perms_,
        region->backing_ == nullptr ? "nullptr" : "exists");
        if ( (addr >= region->addr_) && (addr < (region->addr_ + ((uint64_t) region->length_) ) ) ) {
            return region;
        }
    }
    return nullptr;
}

MemoryRegion* VirtMemMap::findRegion( std::string name )
{
    VirtMemDbg("name=%s\n", name.c_str() );
    auto iter = region_map_.begin();
    for ( ; iter != region_map_.end(); ++iter ) {
        auto region = iter->second;
        VirtMemDbg("region %s [%#" PRIx64 " - %#" PRIx64 "] length=%zu perms=%#x\n",
                region->name_.c_str(),region->addr_,region->addr_+region->length_, region->length_,region->perms_);
        if ( 0 == strcmp( name.c_str(), region->name_.c_str() ) ) {
            return region;
        }
    }
    return nullptr;
}

void VirtMemMap::mprotect( uint64_t addr, size_t length, int prot )
{
    VirtMemDbg("addr=%#" PRIx64 " length=%zu prot=%#x\n", addr, length, prot );
    auto* region = findRegion( addr );
    if ( addr == region->addr_ ) { // mprotect first part of current region
        // split region in two
        if ( length < region->length_ ) {
            // create part two of split
            region_map_[addr+length]
                = new MemoryRegion( region->name_, addr + length, region->length_ - length, region->perms_, region->backing_);
            // update part one of split
            region->length_ = length;
            region->perms_ = prot;
        // mprotect complete region
        } else if ( length == region->length_ ) { // mprotect entire current region
            region->perms_ = prot;
        } else { // we currently are only supporting splitting a region
            assert(0);
        }
    } else if ( ( addr < region->addr_ + region->length_ ) && ( addr + length == region->end() ) ) { // mprotect second part of current region
        region->length_ -= length;
        region_map_[addr] = new MemoryRegion( region->name_, addr, length, prot, region->backing_);
    } else { // we currently are only supporting splitting a region
        assert(0);
    }
}

void VirtMemMap::print(std::string msg) {
#ifdef VIRT_MEM_DBG
    auto iter = region_map_.begin();
    printf("Process VM regions: %s\n",msg.c_str());
    for ( ; iter != region_map_.end(); ++iter ) {
        auto region = iter->second;
        std::string perms;
        if ( region->perms_ & 1<<2 ) {
            perms += "r";
        } else {
            perms += "-";
        }
        if ( region->perms_ & 1<<1 ) {
            perms += "w";
        } else {
            perms += "-";
        }
        if ( region->perms_ & 1<<0 ) {
            perms += "x";
        } else {
            perms += "-";
        }
        printf("%#" PRIx64 "-%#" PRIx64 " %s [%s]\n",region->addr_, region->addr_ + region->length_, perms.c_str(), region->name_.c_str());
    }
    printf("\n");
#endif
}

uint64_t VirtMemMap::mmap( size_t length, uint32_t perms, Device* dev ) {
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

int VirtMemMap::unmap( uint64_t addr, size_t length ) {
    VirtMemDbg("addr=%#" PRIx64 " length=%zu\n", addr, length);

    // Locate the region to which this address belongs
    uint64_t unmap_addr = addr;
    size_t unmap_length = length;
    std::deque<MemoryRegion*> regions;

    while ( unmap_length != 0 ) {
        auto* region = findRegion(unmap_addr);
        if ( nullptr == region ) return EINVAL;

        VirtMemDbg("found region %s() addr=%#" PRIx64 " length=%zu\n",
            region->name_.c_str(), region->addr_, region->length_);

        // Entire region is unmapped
        if ( unmap_addr == region->addr_ && unmap_length >= region->length_ ) {
            unmap_length -= region->length_;
            unmap_addr -= region->length_;
            VirtMemDbg("unmap->removeRegion(%s, 0x%#" PRIx64 ")\n", region->name_, region->addr_);
            removeRegion(region);

        // First part of region is unmapped
        } else if ( unmap_addr == region->addr_ ) {
            // Shrink region to new size
            VirtMemDbg("unmap->shrinkRegion(%s, 0x%#" PRIx64 ", %zu)\n",
                region->name_.c_str(), unmap_addr + unmap_length, region->length_ - unmap_length);
            shrinkRegion(region, unmap_addr + unmap_length, region->length_ - unmap_length);
            unmap_length = 0;

        // second part of region is unmapped - just shrink the existing region
        } else if ( (unmap_addr + unmap_length ) >= region->end() ) {
            VirtMemDbg("unmap->shrinkRegion(%s, 0x%#" PRIx64 ", %zu)\n",
                region->name_.c_str(), region->addr_, region->length_ - unmap_length);
            uint64_t length_removed = region->end() - unmap_addr;
            shrinkRegion(region, region->addr_, unmap_addr - region->addr_);
            unmap_length -= length_removed;
            unmap_addr += length_removed;

        // Need to split the region
        } else {
            size_t new_length = region->end() - (unmap_addr + unmap_length);
            // Shrink current region
            VirtMemDbg("unmap->shrinkRegion(%s, 0x%#" PRIx64 ", %zu)\n",
                region->name_.c_str(), region->addr_, region->length_ - unmap_length);
            shrinkRegion(region, region->addr_, unmap_addr - region->addr_);

            // Create a new region at the end
            VirtMemDbg("unmap->addRegion(%s, 0x%#" PRIx64 ", %zu, %#x, %s)\n",
                region->name_.c_str(), unmap_addr + unmap_length, new_length, region->perms_, region->backing_ ? "backing" : "nullptr");
            addRegion(region->name_, unmap_addr + unmap_length, new_length, region->perms_, region->backing_);
            unmap_length = 0;
        }
    }

    return 0;
}

void VirtMemMap::initBrk( uint64_t addr )
{
    VirtMemDbg("brk=%#" PRIx64 "\n",addr);
    auto start = addRegion( "heap", addr, 0x10000000, 0x6 );
    assert( addr == start );
    heap_region_ = region_map_[addr];
    brk_ = addr;
}

uint64_t VirtMemMap::getBrk()
{
    return brk_;
}

bool VirtMemMap::setBrk( uint64_t brk )
{
    assert( brk >= brk_ );
    assert( brk < heap_region_->addr_ + heap_region_->length_ );
    brk_ = brk;
    return true;
}

void VirtMemMap::snapshot( FILE* fp ) {
    fprintf(fp,"#VirtMemMap start\n");
    fprintf(fp,"m_brk: %#" PRIx64 "\n",brk_);
    fprintf(fp,"m_refCnt: %d\n",ref_cnt_);
    fprintf(fp,"m_regionMap.size() %zu\n",region_map_.size());

    for ( auto & x : region_map_ ) {
        fprintf(fp,"addr: %#" PRIx64 "\n",x.first);
        x.second->snapshot(fp);
    }
    fprintf(fp,"#VirtMemMap end\n");
}

VirtMemMap::VirtMemMap( SST::Output* output, FILE* fp, PhysMemManager* mem_manager, VanadisELFInfo* elf_info) {
    char* str = nullptr;
    size_t num = 0;
    (void) !getline( &str, &num, fp );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"%s",str);
    assert( 0 == strcmp(str,"#VirtMemMap start\n"));
    free(str);

    uint64_t foo;
    assert( 1 == fscanf(fp,"m_brk: %" PRIx64 "\n",&foo));
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"m_brk: %#" PRIx64"\n",foo);

    assert( 1 == fscanf(fp,"m_refCnt: %d\n",&ref_cnt_));
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"m_refCnt: %d\n",ref_cnt_);

    size_t size;
    assert( 1 == fscanf(fp,"m_regionMap.size() %zu\n",&size));
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"m_regionMap.size() %zu\n",size);
    for ( auto i = 0; i < size; i++ ) {
        uint64_t addr;
        assert( 1 == fscanf(fp,"addr: %" PRIx64 "\n",&addr));
        output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"addr: %#" PRIx64 "\n",addr);

        region_map_[addr] = new MemoryRegion( output, fp, mem_manager, elf_info );
    }

    str = nullptr;
    num = 0;
    (void) !getline( &str, &num, fp );
    output->verbose(CALL_INFO, 0, VANADIS_DBG_SNAPSHOT,"%s",str);
    assert( 0 == strcmp(str,"#VirtMemMap end\n"));
    free(str);
}