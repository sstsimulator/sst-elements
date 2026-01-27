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

#include <sst/core/component.h>

#include <math.h>
#include <functional>

#include "os/vnodeos.h"
#include "vanadisDbgFlags.h"
#include "os/vcheckpointreq.h"
#include "os/vgetthreadstate.h"
#include "os/resp/voscallresp.h"
#include "os/resp/vosexitresp.h"
#include "os/voscallev.h"
#include "os/velfloader.h"
#include "os/vstartthreadreq.h"
#include "os/vdumpregsreq.h"
#include "utils.h"
#include "sst/elements/mmu/utils.h"

using namespace SST::Vanadis;

VanadisNodeOSComponent::VanadisNodeOSComponent(SST::ComponentId_t id, SST::Params& params)
    : SST::Component(id), mmu_(nullptr), phys_mem_mgr_(nullptr), current_tid_(100)
{

    const uint32_t verbosity = params.find<uint32_t>("dbgLevel", 0);
    const uint32_t mask = params.find<uint32_t>("dbgMask", 0);

    auto node = params.find<int>("node_id", 0); // Read as default 0 instead of -1 for tagging output_ only
    char* output_prefix = (char*)malloc(sizeof(char) * 256);
    snprintf(output_prefix, sizeof(char)*256, "[node%d-os]:@p():@l ", node);

    output_ = new SST::Output(output_prefix, verbosity, mask, Output::STDOUT);
    free(output_prefix);

    checkpoint_dir_ = params.find<std::string>("checkpointDir", "");
    auto tmp = params.find<std::string>("checkpoint", "");
    if ( ! tmp.empty() ) {
        assert( ! checkpoint_dir_.empty() );
        if ( 0 == tmp.compare( "load" ) ) {
            enable_checkpoint_ = CHECKPOINT_LOAD;
        } else if ( 0 == tmp.compare( "save" ) ) {
            enable_checkpoint_ = CHECKPOINT_SAVE;
        } else {
            assert(0);
        }
    } else {
        enable_checkpoint_ = NO_CHECKPOINT;
    }
    core_count_ = params.find<uint32_t>("cores", 1);
    hardware_thread_count_ = params.find<uint32_t>("hardwareThreadCount", 1);
    logical_core_count_ = core_count_ * hardware_thread_count_;

    if (core_count_ == 0) {
        output_->fatal(CALL_INFO, -1, "Missing parameter (%s): 'cores' must be specified and at least 1.\n", getName().c_str());
    }

    for ( int i = 0; i < core_count_; i++ ) {
        for ( int j = 0; j < hardware_thread_count_; j++ ) {
            avail_hw_threads_.push( new OS::HwThreadID( i,j ) );
        }
    }

    os_start_time_nano_ = params.find<uint64_t>("osStartTimeNano",1000000000);
    process_debug_level_ = params.find<uint32_t>("processDebugLevel",0);
    phdr_address_ = params.find<uint64_t>("program_header_address", 0x60000000);

    // MIPS default is 0x7fffffff according to SYS-V manual
    // we are using it for RISCV as well
    stack_top_ = 0x7ffffff0;

    bool found;
    UnitAlgebra physical_memory_size = UnitAlgebra(params.find<std::string>("physMemSize", "0B", found));

    if ( ! found ) {
        output_->fatal(CALL_INFO, -1, "physMemSize was not specified\n");
    }

    if( 0 == physical_memory_size.getRoundedValue() ) {
        output_->fatal(CALL_INFO, -1, "physMemSize was set to 0B\n");
    }

    page_size_ = params.find<uint32_t>("page_size", 4096);
    if ( !isPowerOfTwo(page_size_) ) {
        output_->fatal(CALL_INFO, -1, "Error: %s requires parameter 'page_size' to be a power of 2. Got '%" PRIu32 "\n",
        getName().c_str(), page_size_ );
    }
    page_shift_ = log2( page_size_ );

    if ( params.find<bool>("useMMU",false) ) { ;
        mmu_ = loadUserSubComponent<SST::MMU_Lib::MMU>("mmu");
        if ( nullptr == mmu_ ) {
            output_->fatal(CALL_INFO, -1, "Error: %s was unable to load subComponent `mmu`\n", getName().c_str());
        }
        MMU_Lib::MMU::Callback callback = std::bind(&VanadisNodeOSComponent::pageFaultHandler, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6,
            std::placeholders::_7, std::placeholders::_8, std::placeholders::_9 );
        mmu_->registerPermissionsCallback( callback );
        phys_mem_mgr_ = new PhysMemManager( physical_memory_size.getRoundedValue());
        // this assumes the first page allocated is physical address 0
        auto zero_page = allocPage( );
        // we don't use it
    }

    node_num_ = params.find<int>("node_id", -1);

    core_info_.resize( core_count_, hardware_thread_count_ );

    int process_number = 0;

    if ( CHECKPOINT_LOAD != enable_checkpoint_ ) {
        while( 1 ) {
            std::string name("process" + std::to_string(process_number) );
            Params tmp = params.get_scoped_params(name);

            if ( ! tmp.empty() ) {
                std::string exe = tmp.find<std::string>("exe", "");

                if ( exe.empty() ) {
                    output_->fatal( CALL_INFO, -1, "--> error - exe is not specified\n");
                }

                auto iter = elf_map_.find( exe );
                if ( iter == elf_map_.end() ) {
                    VanadisELFInfo* elf_info = readBinaryELFInfo(output_, exe.c_str());
                    // readBinaryELFInfo does not return if fatal error is encountered
                    if ( elf_info->isDynamicExecutable() ) {
                        output_->fatal( CALL_INFO, -1, "--> error - exe %s is not statically linked\n",exe.c_str());
                    }
                    elf_map_[exe] = elf_info;
                    #ifdef VANADIS_BUILD_DEBUG
                    elf_info->print(output_);
                    #endif
                }

                unsigned tid = getNewTid();
                thread_map_[tid] = new OS::ProcessInfo( mmu_, phys_mem_mgr_, node_num_, tid, elf_map_[exe], process_debug_level_, page_size_, logical_core_count_, tmp );
                ++process_number;
            } else {
                break;
            }
        }

    } else {
        process_number = checkpointLoad(checkpoint_dir_);
    }

    // make sure we have a thread for each process
    assert( avail_hw_threads_.size() >= thread_map_.size() );

    output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_INIT, "number of process %d\n",process_number);

#ifdef VANADIS_BUILD_DEBUG
    std::string module_name = "vanadisdbg.AppRuntimeMemory";
#else
    std::string module_name = "vanadis.AppRuntimeMemory";
#endif
    module_name += thread_map_.begin()->second->isELF32() ? "32" : "64";
    output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_INIT, "load app runtime memory module: %s\n",module_name.c_str());

    Params not_used;
    app_runtime_memory_ = loadModule<AppRuntimeMemoryMod>(module_name, not_used);

    output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_INIT, "Configuring the memory interface...\n");
    mem_if_ = loadUserSubComponent<Interfaces::StandardMem>(
        "mem_interface", ComponentInfo::SHARE_NONE,
        getTimeConverter("1ps"),
        new StandardMem::Handler2<SST::Vanadis::VanadisNodeOSComponent,&VanadisNodeOSComponent::handleIncomingMemoryCallback>(this));

    if (mem_if_ == nullptr) {
        output_->fatal(CALL_INFO, -1, "Error: failed to load mem interface subcomponent!\n");
    }

    output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_INIT, "Configuring for %" PRIu32 " core links...\n", core_count_);
    core_links_.reserve(core_count_);

    char* port_name_buffer = new char[128];


    for (uint32_t i = 0; i < core_count_; ++i) {
        snprintf(port_name_buffer, 128, "core%" PRIu32 "", i);
        output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_INIT, "---> processing link %s...\n", port_name_buffer);

        SST::Link* core_link = configureLink(
            port_name_buffer, "0ns",
            new Event::Handler2<VanadisNodeOSComponent,&VanadisNodeOSComponent::handleIncomingSyscallEvent>(this));

        if (nullptr == core_link) {
            output_->fatal(CALL_INFO, -1, "Error: unable to configure link: %s\n", port_name_buffer);
        } else {
            output_->verbose(CALL_INFO, 8, VANADIS_OS_DBG_INIT, "configuring link %s...\n", port_name_buffer);
            core_links_.push_back(core_link);
        }
    }

    delete[] port_name_buffer;

    device_list_[-1000] = new OS::Device( "/dev/rdmaNic", 0x80000000, 1048576 );
    // Add balar to vanadis device list
    device_list_[-2000] = new OS::Device( "/dev/balar", 0x80100000, 1024 );

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

VanadisNodeOSComponent::~VanadisNodeOSComponent() {
    delete output_;
    delete phys_mem_mgr_;
}

void
VanadisNodeOSComponent::init(unsigned int phase) {
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_INIT, "Performing init-phase %u...\n", phase);
    #endif
    mem_if_->init(phase);
    if ( nullptr != mmu_ ) {
        mmu_->init(phase);
    }

    // do we need to check for this, really?
    for (Link* next_link : core_links_) {
        while (SST::Event* ev = next_link->recvUntimedData()) {
            assert(0);
        }
    }
}

void
VanadisNodeOSComponent::setup() {

    if ( CHECKPOINT_LOAD == enable_checkpoint_ ) return;

    // start all of the processes
    for ( const auto kv : thread_map_ ) {
        OS::HwThreadID* tmp = avail_hw_threads_.front();
        avail_hw_threads_.pop();

        thread_map_[kv.first]->setHwThread( *tmp );

        startProcess( *tmp, kv.second );
        delete tmp;
    }
}

void
VanadisNodeOSComponent::finish() {


    if ( CHECKPOINT_SAVE == enable_checkpoint_ ) {
        if ( UNLIKELY( ! checkpoint_dir_.empty() ) ) {
            checkpoint( checkpoint_dir_ );
        }
    }
}

void
VanadisNodeOSComponent::checkpoint( std::string dir )
{
    std::stringstream filename;
    filename << dir << "/" << getName();
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"Checkpoint component `%s` %s\n",getName().c_str(), filename.str().c_str());

    auto fp = fopen(filename.str().c_str(),"w+");
    assert(fp);

    mmu_->snapshot( dir );
    phys_mem_mgr_->checkpoint( output_, dir );

    // dump ELF map
    fprintf(fp,"elf_map_.size() %zu\n",elf_map_.size());
    for ( auto & x : elf_map_ ) {
        fprintf(fp,"%s %s\n",x.first.c_str(), x.second->getBinaryPath() );
    }

    // dump the processes
    fprintf(fp,"thread_map_.size() %zu\n", thread_map_.size());
    for ( auto & x : thread_map_ ) {
        // only alow one process
        assert( 100 == x.second->getpid() );
        fprintf(fp,"thread: %d, pid: %d %s\n",x.first,x.second->getpid(), x.second->getElfInfo()->getBinaryPath());
        if ( x.second->getpid() == x.second->gettid() ) {
            x.second->checkpoint( output_, dir );
        }
    }

    fprintf(fp, "core_info_.size() %zu\n",core_info_.size());
    for ( auto i = 0; i < core_info_.size(); i++ ) {
        fprintf(fp, "core: %d\n",i);
        core_info_[i].checkpoint(fp);
    }

    fprintf(fp,"elf_page_cache_.size() %zu\n",elf_page_cache_.size());
    for ( auto & x : elf_page_cache_ ) {
        auto& page_map = x.second;
        fprintf(fp,"filename: %s\n",x.first->getBinaryPath());
        fprintf(fp,"page_map.size(): %zu\n",page_map.size());
        for ( auto & y : page_map ) {
            fprintf(fp,"vpn: %d, ppn: %d, refCnt: %d\n",y.first,y.second->getPPN(), y.second->getRefCnt());
        }
    }

    fprintf(fp,"avail_hw_threads_.size() %zu\n",avail_hw_threads_.size());
    while ( !avail_hw_threads_.empty() ) {
        auto x = avail_hw_threads_.front();
        fprintf(fp,"core: %d, hw_thread: %d\n",x->core,x->hw_thread);
        avail_hw_threads_.pop();
    }

    fprintf(fp,"process_debug_level_: %" PRIu32 "\n",process_debug_level_);
    fprintf(fp,"page_size_: %" PRIu32 "\n",page_size_);
    fprintf(fp,"phdr_address_: %#" PRIx64 "\n",phdr_address_);
    fprintf(fp,"stack_top_: %#" PRIx64 "\n",stack_top_);
    fprintf(fp,"node_num_: %d\n",node_num_);
    fprintf(fp,"os_start_time_nano_: %" PRIu64 "\n",os_start_time_nano_);
    fprintf(fp,"current_tid_: %" PRIu32 "\n",current_tid_);

    assert( pending_fault_.empty() );
    assert( block_memory_write_req_queue_.empty() );
    assert( mem_resp_map_.empty() );
}

int VanadisNodeOSComponent::checkpointLoad( std::string dir )
{
    size_t size;
    int process_number = 0;
    std::stringstream filename;
    filename << checkpoint_dir_ << "/" << getName();
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"Checkpoint component `%s` %s\n",getName().c_str(), filename.str().c_str());

    auto fp = fopen(filename.str().c_str(),"r");
    assert(fp);

    mmu_->snapshotLoad( dir );
    phys_mem_mgr_->checkpointLoad( output_, dir );

    // load ELF map
    assert( 1 == fscanf(fp,"elf_map_.size() %zu\n",&size) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"elf_map_.size() %zu\n",size);
    for ( auto i = 0; i < size; i++ ) {
        char key[80], value[80];
        assert( 2 == fscanf(fp,"%s %s\n",key, value ) );
        output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s %s\n",key,value);

        VanadisELFInfo* elf_info = readBinaryELFInfo(output_, key);
        // readBinaryELFInfo does not return if fatal error is encountered
        if ( elf_info->isDynamicExecutable() ) {
            output_->fatal( CALL_INFO, -1, "--> error - exe %s is not statically linked\n",key);
        }
        elf_map_[key] = elf_info;
    }

    // create processes
    assert( 1 == fscanf(fp,"thread_map_.size() %zu\n", &size) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"thread_map_.size() %zu\n", size);

    std::map<uint32_t,OS::ProcessInfo*> process_map;
    std::map<uint32_t,uint32_t> thread_to_process_map;
    for ( auto i = 0; i < size; i++ ) {
        uint32_t tid, pid;
        char str[80];
        assert( 3 == fscanf(fp,"thread: %" PRIu32 ", pid: %" PRIu32 " %s\n",&tid,&pid,str ) );
        output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"thread: %" PRIu32 ", pid: %" PRIu32 " %s\n",tid,pid, str);
        if ( tid == pid ) {
            thread_map_[tid] = new OS::ProcessInfo( output_, dir, mmu_, phys_mem_mgr_, node_num_, tid, elf_map_[str], process_debug_level_, page_size_, logical_core_count_);
            process_map[pid] = thread_map_[tid];
        } else {
            thread_map_[tid] = new OS::ProcessInfo;
            thread_to_process_map[tid] = pid;
        }
    }

    for ( auto & x : thread_to_process_map ) {
        auto tid = x.first;
        auto pid = x.second;

        *thread_map_[tid ] = *process_map[ pid ];

        thread_map_[tid]->settid(tid);

        process_map[ pid ]->addThread( thread_map_[ tid ] );
    }

    // core_info_.size() 1
    assert( 1 == fscanf(fp,"core_info_.size() %zu\n",&size) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"core_info_.size() %zu\n",size);
    assert( size == core_info_.size() );

    for ( auto i = 0; i < core_info_.size(); i++ ) {

        uint32_t core;
        assert( 1 == fscanf(fp, "core: %" SCNu32 "\n",&core) );
        output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"core: %" PRIu32 "\n",core);
        assert( core == i );

        assert( 1 == fscanf(fp,"hw_thread_map_.size(): %zd\n",&size) );
        output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"hw_thread_map_.size(): %zu\n",size);

        for ( auto j = 0; j < size; j++ ) {
            uint32_t hw_thread;
            // hw_thread: 0
            assert( 1 == fscanf(fp, "hw_thread: %" SCNu32 "\n",&hw_thread) );
            output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"hw_thread: " PRIu32 "\n",hw_thread);
            assert( hw_thread == j );
            uint32_t pid,tid;
            assert ( 2 == fscanf(fp, "pid,tid: %" SCNu32 ", %" SCNu32 "\n",&pid,&tid) );
            output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"pid,tid: %" PRIu32 " %" PRIu32 "\n",pid,tid);
            if ( std::numeric_limits<uint32_t>::max() != pid ) {
                setProcess( i, j, thread_map_[tid] );
            }
        }
    }

    assert ( 1 == fscanf(fp,"elf_page_cache_.size() %zu\n",&size) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"elf_page_cache_.size() %zu\n",size);
    for ( auto i = 0; i < size; i++ ) {
        char str [80];
        assert( 1 == fscanf(fp,"filename: %s\n",str));
        output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"filename: %s\n",str);

        auto & page_map = elf_page_cache_[ elf_map_[str] ];

        size_t size2;
        assert( 1 == fscanf(fp,"page_map.size(): %zu\n", &size2 ) );
        output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"page_map.size(): %zu\n",size2);

        for ( auto j = 0; j < size2; j++ ) {
            int vpn,ppn,refCnt;
            assert( 3 == fscanf(fp,"vpn: %d, ppn: %d, refCnt: %d\n",&vpn, &ppn, &refCnt ) );
            output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"vpn: %d, ppn: %d, refCnt: %d\n",vpn,ppn,refCnt);

            auto region = thread_map_[100]->findMemRegion("text");

            assert( region->backing_ && region->backing_->elf_info_ );
            assert( 0 == strcmp( str, region->backing_->elf_info_->getBinaryPath() ) );

            auto page = region->getPage( vpn );

            assert( refCnt == page->getRefCnt() );
            assert( ppn == page->getPPN() );
            elf_page_cache_[elf_map_[str]][vpn] = page;
        }
    }

    assert( 1 == fscanf(fp,"avail_hw_threads_.size() %zu\n",&size) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"avail_hw_threads_.size() %zu\n",size);
    for ( auto i = 0; i < size; i++ ) {
        int core, hw_thread;
        assert( 2 == fscanf(fp,"core: %d, hw_thread: %d\n",&core,&hw_thread) );
        output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"core: %d, hw_thread: %d\n",core,hw_thread);
        avail_hw_threads_.push(new OS::HwThreadID(core,hw_thread));
    }

    assert( 1 == fscanf(fp,"process_debug_level_: %" SCNu32 "\n",&process_debug_level_) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"process_debug_level_: %" PRIu32 "\n",process_debug_level_);

    assert( 1 == fscanf(fp,"page_size_: %" SCNu32 "\n",&page_size_) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"page_size_: %" PRIu32 "\n",page_size_);

    assert( 1 == fscanf(fp,"phdr_address_: %" SCNx64 "\n",&phdr_address_) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"phdr_address_: %#" PRIx64 "\n",phdr_address_);

    assert( 1 == fscanf(fp,"stack_top_: %" SCNx64 "\n",&stack_top_) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"stack_top_: %#" PRIx64 "\n",stack_top_);

    assert( 1 == fscanf(fp,"node_num_: %d\n",&node_num_) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"node_num_: %d\n",node_num_);

    assert( 1 == fscanf(fp,"os_start_time_nano_: %" PRIu64 "\n",&os_start_time_nano_) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"os_start_time_nano_: %" PRIu64 "\n",os_start_time_nano_);

    assert( 1 == fscanf(fp,"current_tid_: %" SCNu32 "\n",&current_tid_) );
    output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"current_tid_: %" PRIu32 "\n",current_tid_);

//    exit(0);
    return thread_map_.size();
}

void VanadisNodeOSComponent::handleIncomingMemoryCallback(StandardMem::Request* ev) {
    auto lookup_result = mem_resp_map_.find(ev->getID());

    if ( lookup_result == mem_resp_map_.end() )  {
        if ( ! block_memory_write_req_queue_.empty( ) ) {
            auto req = block_memory_write_req_queue_.front();
            try {
                if ( req->handleResp( ev ) ) {
                    delete req;
                    block_memory_write_req_queue_.pop();
                    if ( ! block_memory_write_req_queue_.empty() ) {
                        startBlockXfer( block_memory_write_req_queue_.front());
                    }
                }
            } catch ( int e ) {
                output_->fatal(CALL_INFO, -1, "Error - received StandardMem response that does not match PageWrite request\n");
            }
        } else {
//            output_->fatal(CALL_INFO, -1, "Error - received StandardMem response that does not belong to a core\n");
            flush_pages_.pop_front();
            if ( flush_pages_.empty() ) {
                primaryComponentOKToEndSim();
            }

        }
    } else if (lookup_result != mem_resp_map_.end()) {
        handleIncomingMemory( lookup_result->second, ev );
        mem_resp_map_.erase(lookup_result);
    } else {
        assert(0);
    }
}

void VanadisNodeOSComponent::copyPage( uint64_t phys_from, uint64_t phys_to, uint32_t page_size, Callback* callback )
{
    auto data = new uint8_t[page_size];
    Callback* tmp = new Callback( [=](){
        writePage( phys_to, data, page_size, callback );
    });
    readPage( phys_from, data, page_size, tmp );
}

void
VanadisNodeOSComponent::startProcess( OS::HwThreadID& thread_id, OS::ProcessInfo* process )
{
    int pid = process->getpid();

    if ( mmu_ ) {
        mmu_->initPageTable( pid );
        mmu_->setCoreToPageTable( thread_id.core, thread_id.hw_thread, pid );
    }

    OS::MemoryBacking* phdr_backing = new OS::MemoryBacking;
    uint64_t rand_values_address = app_runtime_memory_->configurePhdr( output_, page_size_, process, phdr_address_, phdr_backing->data_ );
    // configurePhdr() should have returned a block of memory that is a multiple of a page size
    assert( 0 == phdr_backing->data_.size() % page_size_ );
    phdr_backing->data_start_addr_ = phdr_address_;

    size_t  phdr_region_end = phdr_address_ + phdr_backing->data_.size();
    // setup a VM memory region for this process
    process->addMemRegion( "phdr", phdr_address_, phdr_region_end - phdr_address_, 0x4, phdr_backing );

    OS::MemoryBacking* stack_backing = new OS::MemoryBacking;
    uint64_t stack_pointer = app_runtime_memory_->configureStack( output_, page_size_, process, stack_top_, phdr_address_, rand_values_address, stack_backing->data_ );
    // configureStack() should have returned a block of memory that is a multiple of a page size
    assert( 0 == stack_backing->data_.size() % page_size_ );
    uint64_t aligned_stack_address = stack_pointer & ~(page_size_-1);

    stack_backing->data_start_addr_ = aligned_stack_address;

    // stack vm region start right after the phdrs
    uint64_t stack_region_end = aligned_stack_address + stack_backing->data_.size();
    process->addMemRegion( "stack", phdr_region_end, stack_region_end - phdr_region_end, 0x6, stack_backing );

    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose( CALL_INFO, 16, VANADIS_OS_DBG_APP_INIT,
        "stack_pointer=%#" PRIx64 " stack_memory_region_start=%#" PRIx64" stack_region_length=%" PRIu64 "\n",
        stack_pointer, (uint64_t) phdr_region_end, stack_region_end - phdr_region_end);
    #endif
    process->printRegions("after app runtime setup");

    core_info_.at(thread_id.core).setProcess( thread_id.hw_thread, process );

    uint64_t entry = process->getEntryPoint();
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_APP_INIT,
        "stack_pointer=%#" PRIx64 " entry=%#" PRIx64 "\n",stack_pointer, entry );
    #endif
    core_links_.at(thread_id.core)->send( new VanadisStartThreadFirstReq( thread_id.hw_thread, entry, stack_pointer ) );
}

void VanadisNodeOSComponent::writeMem( OS::ProcessInfo* process, uint64_t virt_addr, std::vector<uint8_t>* data, uint32_t perms, unsigned page_size, Callback* callback )
{
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 8, VANADIS_OS_DBG_PAGE_FAULT,"virt_addr=%#" PRIx64 " length=%zu perm=%x\n",virt_addr,data->size(), perms);
    #endif
    OS::Page* page;
    try {
        page = allocPage( );
    } catch ( int err ) {
        output_->fatal(CALL_INFO, -1, "Error: ran out of physical memory\n");
    }

    uint32_t vpn = virt_addr >> page_shift_;

    process->mapVirtToPage( vpn, page );

    // map this physical page into the MMU for this process
    mmu_->map( process->getpid(), vpn, page->getPPN(), page_size_, perms);
    auto tmp = new uint8_t[page_size_];
    memcpy( tmp, data->data(), data->size() );
    writePage( page->getPPN() << page_shift_, tmp, page_size_, callback );
}

void
VanadisNodeOSComponent::handleIncomingSyscallEvent(SST::Event* ev) {
    VanadisSyscallEvent* sys_ev = dynamic_cast<VanadisSyscallEvent*>(ev);

    if (nullptr == sys_ev) {
        VanadisCoreEvent* event = dynamic_cast<VanadisCoreEvent*>(ev);

        if ( nullptr != event ) {
            auto syscall = getSyscall( event->getCore(), event->getThread() );
            syscall->handleEvent( event );
            processSyscallPost( syscall );
        } else {

            VanadisCheckpointResp* resp = dynamic_cast< VanadisCheckpointResp*>(ev);
            if ( nullptr != resp ) {
                output_->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"checkpoint \n");
             //   primaryComponentOKToEndSim();

                flush_pages_.push_back( 0x2bc0 );
                flush_pages_.push_back( 0x163c0 );
                flush_pages_.push_back( 0x16400 );
                flush_pages_.push_back( 0x90c0 );
                flush_pages_.push_back( 0x2c80 );
                flush_pages_.push_back( 0x2c00 );
                flush_pages_.push_back( 0x1b00 );

                flush_pages_.push_back( 0x1b000 );
                flush_pages_.push_back( 0x2b80 );
                flush_pages_.push_back( 0x2980 );
                flush_pages_.push_back( 0x2a00 );
                flush_pages_.push_back( 0x2940 );
                flush_pages_.push_back( 0x2900 );
                flush_pages_.push_back( 0x3f80 );
                flush_pages_.push_back( 0x2800 );
                flush_pages_.push_back( 0x29c0 );
                flush_pages_.push_back( 0x28c0 );

                for ( auto & x : flush_pages_ ) {
                    printf("%#" PRIx64 "\n",x);
                    StandardMem::Request* req = new SST::Interfaces::StandardMem::FlushAddr( x, 64, true, 5, 0 );
                    mem_if_->send(req);
                }
            } else {
                output_->fatal(CALL_INFO, -1,
                      "Error - received an event in the OS, but cannot cast it to "
                      "a system-call event.\n");
            }
        }
    } else {
	    auto process = core_info_.at(sys_ev->getCoreID()).getProcess( sys_ev->getThreadID() );
	    if ( process ) {
		    auto syscall = handleIncomingSyscall( process, sys_ev, core_links_[ sys_ev->getCoreID() ] );

		    processSyscallPost( syscall );
	    } else {
            // Probable error, do not ifdef
		    output_->output("%s: no active process for core %d, hwthread %d\n", getName().c_str(), sys_ev->getCoreID(), sys_ev->getThreadID() );
		    delete ev;
	    }
    }
}

void VanadisNodeOSComponent::processSyscallPost( VanadisSyscall* syscall ) {

    auto core = syscall->getCoreId();
    auto hw_thread = syscall->getThreadId();
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, 0,"syscall '%s' for core %d\n",syscall->getName().c_str(),core);
    #endif
    if ( syscall->isComplete() ) {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, 0,"syscall '%s' for core %d has finished\n",syscall->getName().c_str(),core);
        #endif
        delete syscall;

    } else {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, 0,"syscall '%s' for core %d get memory reqeust\n",syscall->getName().c_str(),core);
        #endif
        auto ev = syscall->getMemoryRequest();

        if ( ev ) {
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 16, 0,"syscall '%s' for core %d has a memory request\n",syscall->getName().c_str(),core);
            #endif
            sendMemoryEvent(syscall, ev );
        } else if ( syscall->causedPageFault() ) {
            uint64_t virt_addr;
            bool is_write;
            std::tie( virt_addr, is_write) = syscall->getPageFault();
            processOsPageFault( syscall, virt_addr, is_write );
        #ifdef VANADIS_BUILD_DEBUG
        } else {
            output_->verbose(CALL_INFO, 16, 0,"syscall '%s' for core %d is blocked\n",syscall->getName().c_str(),core);
        #endif
        }
    }
}

void VanadisNodeOSComponent::processOsPageFault( VanadisSyscall* syscall, uint64_t virt_addr, bool is_write ) {
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT, "virt_addr=%#" PRIx64 " is_write=%d\n",virt_addr, is_write);
    #endif

    uint32_t vpn = virt_addr >> page_shift_;
    uint32_t fault_perms = is_write ? 1 << 1:  1<< 2;
    uint32_t uninit = std::numeric_limits<uint32_t>::max();
    pageFaultHandler2( uninit, uninit, uninit, uninit, syscall->getPid(), vpn, fault_perms, 0, virt_addr, syscall );
}

void VanadisNodeOSComponent::pageFaultHandler2( MMU_Lib::RequestID req_id, uint32_t link, uint32_t core, uint32_t hw_thread,
                uint32_t pid,  uint32_t vpn, uint32_t fault_perms, uint64_t inst_ptr, uint64_t mem_virt_addr, VanadisSyscall* syscall )
{
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT, "RequestID=%#" PRIx64 " link=%d pid=%d vpn=%d perms=%#x inst_ptr=%#" PRIx64 " syscall=%p\n",
            req_id, link, pid, vpn, fault_perms, inst_ptr, syscall );
    #endif

    auto tmp = new PageFault( req_id, link, core, hw_thread, pid, vpn, fault_perms, inst_ptr, mem_virt_addr, syscall );
    pending_fault_.push( tmp );
    if ( 1 == pending_fault_.size() ) {
        pageFault( tmp );
    #ifdef VANADIS_BUILD_DEBUG
    } else {
        output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT, "queue page fault\n" );
    #endif
    }
}

void VanadisNodeOSComponent::pageFaultFini( PageFault* info, bool success )
{
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"link=%d pid=%" PRIu32 " vpn=%" PRIu32 " %#" PRIx64 " %s\n",
                info->link, info->pid, info->vpn, (uint64_t)(info->vpn) << page_shift_, success ? "success":"fault" );
    #endif
    if( info->syscall ) {
        auto ev = info->syscall->getMemoryRequest();
        assert(ev);
        sendMemoryEvent(info->syscall, ev );
    } else {
        mmu_->faultHandled( info->req_id, info->link, info->pid, info->vpn, success );
    }
    delete info;

    pending_fault_.pop();
    if ( pending_fault_.size() ) {
        auto tmp = pending_fault_.front();
        pageFault( tmp );
    }
}

void VanadisNodeOSComponent::pageFault( PageFault *info )
{
    MMU_Lib::RequestID req_id = info->req_id;
    uint32_t link = info->link;
    uint32_t pid = info->pid;
    uint32_t vpn = info->vpn;
    uint32_t fault_perms = info->fault_perms;

    assert(pid > 0);
    if ( thread_map_.find(pid) == thread_map_.end() ) {
        output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"process %" PRIu32 " is gone, wanted vpn=%" PRIu32 " pass error back to CPU\n",pid,vpn);
        pageFaultFini( info, false );
        return;
    }
    auto thread = thread_map_.at(pid);
    uint64_t virt_addr = ((uint64_t)vpn) << page_shift_;

    // this is confusing because we have two virtAddrs: one is the page virtaddr and the other is the address of the memory req that faulted,
    // the full address was added for debug, we should get rid of the VPN at some point.
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"link=%" PRIu32 " pid=%" PRIu32 " virt_addr=%#" PRIx64 " %c%c%c inst_ptr=%#" PRIx64 " virtMemAddr=%#" PRIx64 "\n",
            link,pid,virt_addr,
            fault_perms & 0x4 ? 'R' : '-',
            fault_perms & 0x2 ? 'W' : '-',
            fault_perms & 0x1 ? 'X' : '-',
            info->inst_ptr,
            info->mem_virt_addr);
    #endif
    auto region = thread->findMemRegion( virt_addr + 1 );

    if ( region ) {
        // std::numeric_limits::max indicates the vpn is not mapped to a physical page
        uint32_t page_perms = mmu_->getPerms( pid, vpn);

        // We got here because a TLB has to have an address resolved.
        // There are two cases that can happen:
        // 1) Read or Write from a data tlb
        // 2) Read with Execute from a inst tlb

        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"found region %#" PRIx64 "-%#" PRIx64 "\n",region->addr_,region->addr_ + region->length_);

        output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"virt_addr=%#010" PRIx64 ": fault-perms %c%c%c, VM-perms %c%c%c page_perms=%#x\n",virt_addr,
            fault_perms & 0x4 ? 'R' : '-',
            fault_perms & 0x2 ? 'W' : '-',
            fault_perms & 0x1 ? 'X' : '-',
            region->perms_ & 0x4 ? 'R' : '-',
            region->perms_ & 0x2 ? 'W' : '-',
            region->perms_ & 0x1 ? 'X' : '-',
            page_perms);
        #endif
        // if the page is present the fault wants to write but the page doesn't have write, could be COW
        if( (page_perms != std::numeric_limits<uint32_t>::max()) && (fault_perms & 0x2 &&  0 == (page_perms & 0x2 ) ) ) {
            OS::Page* new_page;
            uint32_t orig_ppn = mmu_->virtToPhys(pid,vpn);
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"COW ppn of origin page %" PRIu32 "\n",orig_ppn);
            #endif
            // check if we can upgrade the permission for this page
            if ( ! MMU_Lib::checkPerms( fault_perms, region->perms_ ) ) {
                #ifdef VANADIS_BUILD_DEBUG
                output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"core %" PRIu32 ", hw_thread %" PRIu32 ", inst_ptr %#" PRIx64 " caused page fault at address %#" PRIx64 "\n",
                    info->core,info->hw_thread,info->inst_ptr,info->mem_virt_addr);
                #endif
                pageFaultFini( info, false );
                return;
            }

            try {
                new_page = allocPage( );
            } catch ( int err ) {
                output_->fatal(CALL_INFO, -1, "Error: ran out of physical memory\n");
            }

            thread->mapVirtToPage( vpn, new_page );

            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"origin ppn %" PRIu32 " new ppn %" PRIu32 "\n",orig_ppn, new_page->getPPN());
            #endif
            // map this physical page into the MMU for this process with the regions permissions
            mmu_->map( thread->getpid(), vpn, new_page->getPPN(), page_size_, region->perms_ );

            auto callback = new Callback( [=]() {
                pageFaultFini( info );
            });
            copyPage( ((uint64_t)orig_ppn) << page_shift_, ((uint64_t)new_page->getPPN()) << page_shift_, page_size_, callback );
            return;
        }

        // the fault is in a present region, check to see if the retions permissions satisfy the fault
        if ( ! MMU_Lib::checkPerms( fault_perms, region->perms_ ) ) {
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT, "memory fault inst_ptr=%#" PRIx64 ", could not be satified for %#" PRIx64 ", no permission wantPerms=%#x havePerms=%#x\n",
                    info->inst_ptr,virt_addr,fault_perms,region->perms_);
            #endif
            pageFaultFini( info, false );
            return;
        }

        uint32_t page_table_perms =  mmu_->getPerms( pid, vpn );
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"vpn %" PRIu32 " perms %#x\n",vpn,page_table_perms);
        #endif
        if ( page_table_perms != std::numeric_limits<uint32_t>::max() ) {
            if ( ! MMU_Lib::checkPerms( fault_perms, region->perms_ ) ) {
                #ifdef VANADIS_BUILD_DEBUG
                output_->verbose(CALL_INFO, 1, 0,"core %" PRIu32 ", hw_thread %" PRIu32 ", inst_ptr %#" PRIx64 " caused page fault at address %#" PRIx64 "\n",
                    info->core,info->hw_thread,info->inst_ptr,info->mem_virt_addr);
                #endif
                pageFaultFini( info, false );
                return;
            }
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"using existing page vpn=%" PRIu32 "\n",vpn);
            #endif
            pageFaultFini( info );
            return;
        }

        OS::Page* page = nullptr;

        uint8_t* data = nullptr;
        // if this region has backing
        if ( region->backing_ ) {
            // if this region is mapped to an ELF file, check to see if the physical page is cached
            if ( region->backing_->elf_info_ ) {
                page = checkPageCache( region->backing_->elf_info_, vpn );
                if ( nullptr == page ) {
                    data = readElfPage( output_, region->backing_->elf_info_, vpn, page_size_ );
                }
            } else if ( region->backing_->dev_ ) {
                // map this physical page into the MMU for this process
                auto phys_addr = region->backing_->dev_->getPhysAddr();
                auto offset = vpn - ( region->addr_ >> page_shift_);
                auto ppn = ( phys_addr >> page_shift_ ) + offset;
                #ifdef VANADIS_BUILD_DEBUG
                output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT, "Device physAddr=%#" PRIx64 " ppn=%" PRIu64 "\n",phys_addr,ppn);
                #endif
                mmu_->map( thread->getpid(), vpn, ppn, page_size_, region->perms_ );
                pageFaultFini( info );
                return;
            } else {
                assert( region->backing_->data_.size());
                data = region->readData( vpn << page_shift_, page_size_ );
            }
        }

        // if there is no physical backing for this virtual page, get a page
        if ( nullptr == page ) {
            try {
                page = allocPage( );
            } catch ( int err ) {
                output_->fatal(CALL_INFO, -1, "Error: ran out of physical memory\n");
            }
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"alloced physical page %d\n", page->getPPN() );
            #endif
            thread->mapVirtToPage( vpn, page );
        } else {
            page->incRefCnt();
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"using exiting physical page %d\n",page->getPPN());
            #endif
        }

        // map this physical page into the MMU for this process
        mmu_->map( thread->getpid(), vpn, page->getPPN(), page_size_, region->perms_ );

        // if there's elf_info for this region is mapped to a file update the page cache
        if ( region->backing_ && region->backing_->elf_info_ && 0 == region->name_.compare("text") ) {
            if ( nullptr != data ) {
                updatePageCache( region->backing_->elf_info_, vpn, page );
            } else {
                #ifdef VANADIS_BUILD_DEBUG
                output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"fault handled link=%d pid=%d vpn=%d %#" PRIx32 " ppn=%d\n",link,pid,vpn, vpn << page_shift_,page->getPPN());
                #endif
                pageFaultFini( info );
                return;
            }
        }

        auto callback = new Callback( [=]() {
            pageFaultFini( info );
        });

        if ( nullptr == data ) {
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"zero page\n");
            #endif
            data = new uint8_t[page_size_];
            bzero( data, page_size_ );
        }
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"write page\n");
        #endif
        writePage( page->getPPN() << page_shift_, data, page_size_, callback );
    } else {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 1, VANADIS_OS_DBG_PAGE_FAULT,"core %d, hw_thread %d, inst_ptr %#" PRIx64 " caused page fault at address %#" PRIx64 "\n",
            info->core, info->hw_thread, info->inst_ptr, info->mem_virt_addr );
        #endif
        pageFaultFini( info, false );
    }
}

bool VanadisNodeOSComponent::PageMemReadReq::handleResp( StandardMem::Request* ev ) {

    //printf("PageMemReadReq::%s()\n",__func__);
    auto iter = req_map_.find( ev->getID() );
    assert ( iter != req_map_.end() );

    StandardMem::ReadResp* req = dynamic_cast<StandardMem::ReadResp*>(ev);
    assert( req );
    assert( req->size == 64 );

    memcpy( data_ + iter->second, req->data.data(), req->size );
    req_map_.erase( iter );

    delete ev;

    if ( offset_ < length_ ) {
        sendReq();
    }

    return req_map_.empty();
}

void VanadisNodeOSComponent::PageMemReadReq::sendReq() {

    //printf("PageMemReadReq::%s()\n",__func__);
    if ( current_req_offset_ < length_ ) {
        StandardMem::Request* req = new SST::Interfaces::StandardMem::Read( addr_ + current_req_offset_, 64 );
        req_map_[req->getID()] = current_req_offset_;
        current_req_offset_ += 64;
        mem_if_->send(req);
    }
}

bool VanadisNodeOSComponent::PageMemWriteReq::handleResp( StandardMem::Request* ev ) {
    //printf("PageMemWriteReq::%s()\n",__func__);
    auto iter = req_map_.find( ev->getID() );
    assert ( iter != req_map_.end() );
    req_map_.erase( iter );
    delete ev;
    if ( offset_ < length_ ) {
        sendReq();
    }
    return req_map_.empty();
}

void VanadisNodeOSComponent::PageMemWriteReq::sendReq() {
    //printf("PageMemWriteReq::%s()\n",__func__);
    if ( offset_ < length_ ) {
        std::vector< uint8_t > buffer( 64);

        memcpy( buffer.data(), data_ + offset_, buffer.size() );
        StandardMem::Request* req = new SST::Interfaces::StandardMem::Write( addr_ + offset_, buffer.size(), buffer );
        req_map_[req->getID()] = offset_;
        offset_ += buffer.size();
        mem_if_->send(req);
    }
}
