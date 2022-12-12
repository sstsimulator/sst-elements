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

#include <math.h>
#include <sst_config.h>
#include <sst/core/component.h>

#include <functional>

#include "os/vgetthreadstate.h"
#include "os/resp/voscallresp.h"
#include "os/resp/vosexitresp.h"
#include "os/vnodeos.h"
#include "os/voscallev.h"
#include "os/velfloader.h"
#include "os/vstartthreadreq.h"
#include "os/vdumpregsreq.h"
#include "sst/elements/mmu/utils.h"

using namespace SST::Vanadis;

VanadisNodeOSComponent::VanadisNodeOSComponent(SST::ComponentId_t id, SST::Params& params) 
    : SST::Component(id), m_mmu(nullptr), m_physMemMgr(nullptr), m_currentTid(100) 
{

    const uint32_t verbosity = params.find<uint32_t>("verbose", 0);
    output = new SST::Output("[node-os]:@p():@l ", verbosity, 0, SST::Output::STDOUT);

    const uint32_t core_count = params.find<uint32_t>("cores", 0);
    const uint32_t hardwareThreadCount = params.find<uint32_t>("hardwareThreadCount", 1);

    for ( int i = 0; i <core_count; i++ ) {
        for ( int j = 0; j <hardwareThreadCount; j++ ) {
            m_availHwThreads.push( new OS::HwThreadID( i,j ) );
        } 
    } 

    m_processDebugLevel = params.find<uint32_t>("processDebugLevel",0);
    m_phdr_address = params.find<uint64_t>("program_header_address", 0x60000000);

    // MIPS default is 0x7fffffff according to SYS-V manual
    // we are using it for RISCV as well
    m_stack_top = 0x7ffffff0;

    bool found;
    UnitAlgebra physMemSize = UnitAlgebra(params.find<std::string>("physMemSize", "0B", found));

    if ( ! found ) {
        output->fatal(CALL_INFO, -1, "physMemSize was not specifed\n");
    }

    if( 0 == physMemSize.getRoundedValue() ) {
        output->fatal(CALL_INFO, -1, "physMemSize was to 0\n");
    }

    m_pageSize = params.find<uint64_t>("page_size", 4096);
    m_pageShift = log2( m_pageSize );

    if ( params.find<bool>("useMMU",false) ) { ;
        m_mmu = loadUserSubComponent<SST::MMU_Lib::MMU>("mmu");
        if ( nullptr == m_mmu ) {
            output->fatal(CALL_INFO, -1, "Error: was unable to load subComponent `mmu`\n");
        }
        MMU_Lib::MMU::Callback callback = std::bind(&VanadisNodeOSComponent::pageFaultHandler, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, 
            std::placeholders::_7, std::placeholders::_8, std::placeholders::_9 );
        m_mmu->registerPermissionsCallback( callback );
        m_physMemMgr = new PhysMemManager( physMemSize.getRoundedValue()); 
        // this assumes the first page allocated is physical address 0
        auto zeroPage = allocPage( );
        // we don't use it
    }

    int numProcess = 0;
    while( 1 ) {
        std::string name("process" + std::to_string(numProcess) );
        Params tmp = params.get_scoped_params(name);

        if ( ! tmp.empty() ) {
            std::string exe = tmp.find<std::string>("exe", "");

            if ( exe.empty() ) {
                output->fatal( CALL_INFO, -1, "--> error - exe is not specified\n");
            }

            auto iter = m_elfMap.find( exe );
            if ( iter == m_elfMap.end() ) {
                VanadisELFInfo* elfInfo = readBinaryELFInfo(output, exe.c_str());
                // readBinaryELFInfo does not return if fatal error is encountered
                if ( elfInfo->isDynamicExecutable() ) {
                    output->fatal( CALL_INFO, -1, "--> error - exe %s is not staticlly linked\n",exe.c_str());
                }
                m_elfMap[exe] = elfInfo;
            }

            unsigned tid = getNewTid();
            m_threadMap[tid] = new OS::ProcessInfo( m_mmu, m_physMemMgr, tid, m_elfMap[exe], m_processDebugLevel, m_pageSize, tmp );
            ++numProcess;
        } else {
          break;
        }
    }

    // make sure we have a thread for each process
    assert( m_availHwThreads.size() >= m_threadMap.size() );

    output->verbose(CALL_INFO, 1, 0, "number of process %d\n",numProcess);

    std::string modName = "vanadis.AppRuntimeMemory"; 
    modName += m_threadMap.begin()->second->isELF32() ? "32" : "64";
    output->verbose(CALL_INFO, 1, 0, "load app runtime memory module: %s\n",modName.c_str());

    Params notUsed;
    m_appRuntimeMemory = loadModule<AppRuntimeMemoryMod>(modName,notUsed);

    output->verbose(CALL_INFO, 1, 0, "Configuring the memory interface...\n");
    mem_if = loadUserSubComponent<Interfaces::StandardMem>("mem_interface", ComponentInfo::SHARE_NONE,
                                                         getTimeConverter("1ps"),
                                                         new StandardMem::Handler<SST::Vanadis::VanadisNodeOSComponent>(
                                                             this, &VanadisNodeOSComponent::handleIncomingMemory));
    output->verbose(CALL_INFO, 1, 0, "Configuring for %" PRIu32 " core links...\n", core_count);
    core_links.reserve(core_count);

    char* port_name_buffer = new char[128];

    uint64_t heap_start = params.find<uint64_t>("heap_start", 0);
    uint64_t heap_end = params.find<uint64_t>("heap_end", 0);
    int heap_verbose = params.find<int>("heap_verbose", 0);

    output->verbose(CALL_INFO, 1, 0, "-> configuring mmap page range start: 0x%llx\n", heap_start);
    output->verbose(CALL_INFO, 1, 0, "-> configuring mmap page range end:   0x%llx\n", heap_end);
    output->verbose(CALL_INFO, 1, 0, "-> implies:                           %" PRIu64 " pages\n", (heap_end - heap_start) / m_pageSize);
    output->verbose(CALL_INFO, 1, 0, "-> configuring mmap page size:        %d bytes\n", m_pageSize);

    m_coreInfoMap.resize( core_count, hardwareThreadCount ); 

    for (uint32_t i = 0; i < core_count; ++i) {
        snprintf(port_name_buffer, 128, "core%" PRIu32 "", i);
        output->verbose(CALL_INFO, 1, 0, "---> processing link %s...\n", port_name_buffer);

        SST::Link* core_link = configureLink(
            port_name_buffer, "0ns",
            new Event::Handler<VanadisNodeOSComponent>(this, &VanadisNodeOSComponent::handleIncomingSyscall));

        if (nullptr == core_link) {
            output->fatal(CALL_INFO, -1, "Error: unable to configure link: %s\n", port_name_buffer);
        } else {
            output->verbose(CALL_INFO, 8, 0, "configuring link %s...\n", port_name_buffer);
            core_links.push_back(core_link);
        }
    }

    m_sendMemReqFunc = std::bind(&VanadisNodeOSComponent::sendMemoryEvent, this, std::placeholders::_1, std::placeholders::_2);

    delete[] port_name_buffer;

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

VanadisNodeOSComponent::~VanadisNodeOSComponent() {
    delete output;
    delete m_physMemMgr;
}

void
VanadisNodeOSComponent::init(unsigned int phase) {
    output->verbose(CALL_INFO, 1, 0, "Performing init-phase %u...\n", phase);
    mem_if->init(phase);
    if ( nullptr != m_mmu ) {
        m_mmu->init(phase);
    }

    // do we need to check for this, really?
    for (Link* next_link : core_links) {
        while (SST::Event* ev = next_link->recvInitData()) {
            assert(0);
        }
    }
}

void
VanadisNodeOSComponent::setup() {
    // start all of the processes
    for ( const auto kv : m_threadMap ) {
        OS::HwThreadID* tmp = m_availHwThreads.front();
        m_availHwThreads.pop();

        m_threadMap[kv.first]->setHwThread( *tmp );

        startProcess( *tmp, kv.second ); 
        delete tmp;
    }
}

void VanadisNodeOSComponent::handleIncomingMemory(StandardMem::Request* ev) {
    auto lookup_result = m_memRespMap.find(ev->getID());

    if ( lookup_result == m_memRespMap.end() )  {
        if ( ! m_blockMemoryWriteReqQ.empty( ) ) {
            auto req = m_blockMemoryWriteReqQ.front();
            try {
                if ( req->handleResp( ev ) ) {
                    delete req;
                    m_blockMemoryWriteReqQ.pop();
                    if ( ! m_blockMemoryWriteReqQ.empty() ) {
                        startFoo( m_blockMemoryWriteReqQ.front());
                    }
                }
            } catch ( int e ) {
                output->fatal(CALL_INFO, -1, "Error - received StandardMem response that does not match PageWrite request\n");
            }
        } else {
            output->fatal(CALL_INFO, -1, "Error - received StandardMem response that does not belong to a core\n");
        }
    } else if (lookup_result != m_memRespMap.end()) {
        handleIncomingMemory( lookup_result->second, ev );
        m_memRespMap.erase(lookup_result);
    } else {
        assert(0);
    }
}

void VanadisNodeOSComponent::copyPage( uint64_t physFrom, uint64_t physTo, unsigned pageSize,Callback* callback )
{
    auto data = new uint8_t[m_pageSize];
    Callback* tmp = new Callback( [=](){
        writePage( physTo, data, pageSize, callback );
    });
    readPage( physFrom, data, pageSize, tmp );
}

void 
VanadisNodeOSComponent::startProcess(unsigned core, unsigned hwThread, unsigned pid, VanadisStartThreadFullReq* req ) 
{
    output->verbose(CALL_INFO, 2, 0,"core=%d hwThread=%d \n",core,hwThread);
    core_links.at(core)->send( req );
}

void
VanadisNodeOSComponent::startProcess( OS::HwThreadID& threadID, OS::ProcessInfo* process ) 
{
    int pid = process->getpid();

    if ( m_mmu ) {
        m_mmu->initPageTable( pid );
        m_mmu->setCoreToPageTable( threadID.core, threadID.hwThread, pid );
    }

    OS::MemoryBacking* phdrBacking = new OS::MemoryBacking;
    uint64_t rand_values_address = m_appRuntimeMemory->configurePhdr( output, m_pageSize, process, m_phdr_address, phdrBacking->data );
    // configurePhdr() should have returned a block of memory that is a multiple of a page size
    assert( 0 == phdrBacking->data.size() % m_pageSize );
    phdrBacking->dataStartAddr = m_phdr_address;

    size_t  phdrRegionEnd = m_phdr_address + phdrBacking->data.size();
    // setup a VM memory region for this process
    process->addMemRegion( "phdr", m_phdr_address, phdrRegionEnd - m_phdr_address, 0x4, phdrBacking );

    OS::MemoryBacking* stackBacking = new OS::MemoryBacking;
    uint64_t stack_pointer = m_appRuntimeMemory->configureStack( output, m_pageSize, process, m_stack_top, m_phdr_address, rand_values_address, stackBacking->data );
    // configureStack() should have returned a block of memory that is a multiple of a page size
    assert( 0 == stackBacking->data.size() % m_pageSize );
    uint64_t aligned_stack_address = stack_pointer & ~(m_pageSize-1);

    stackBacking->dataStartAddr = aligned_stack_address;

    // stack vm region start right after the phdrs
    uint64_t stackRegionEnd = aligned_stack_address + stackBacking->data.size();
    process->addMemRegion( "stack", phdrRegionEnd, stackRegionEnd - phdrRegionEnd, 0x6, stackBacking );

    output->verbose( CALL_INFO, 16, 0, "stack_pointer=%#" PRIx64 " stack_memory_region_start=%#" PRIx64" stack_region_length=%" PRIu64 "\n",
                                                        stack_pointer, phdrRegionEnd, stackRegionEnd - phdrRegionEnd);

    process->printRegions("after app runtime setup");

    m_coreInfoMap.at(threadID.core).setProcess( threadID.hwThread, process );

    uint64_t entry = process->getEntryPoint();
    output->verbose(CALL_INFO, 1, 0, "stack_pointer=%#" PRIx64 " entry=%#" PRIx64 "\n",stack_pointer, entry );
    
    core_links.at(threadID.core)->send( new VanadisStartThreadReq( threadID.hwThread, stack_pointer, entry ) );
}

void VanadisNodeOSComponent::writeMem( OS::ProcessInfo* process, uint64_t virtAddr, std::vector<uint8_t>* data, int perms, unsigned pageSize, Callback* callback )
{
    output->verbose(CALL_INFO, 8, 0,"virtAddr=%#" PRIx64 " length=%zu perm=%x\n",virtAddr,data->size(), perms);

    OS::Page* page;
    try {
        page = allocPage( );
    } catch ( int err ) {
        output->fatal(CALL_INFO, -1, "Error: ran out of physical memory\n");
    }

    unsigned vpn = virtAddr >> m_pageShift; 

    process->mapVirtToPage( vpn, page );

    // map this physical page into the MMU for this process 
    m_mmu->map( process->getpid(), vpn, page->getPPN(), m_pageSize, perms);
    auto tmp = new uint8_t[m_pageSize];
    memcpy( tmp, data->data(), data->size() );
    writePage( page->getPPN() << m_pageShift, tmp, m_pageSize, callback );
}

void
VanadisNodeOSComponent::handleIncomingSyscall(SST::Event* ev) {
    VanadisSyscallEvent* sys_ev = dynamic_cast<VanadisSyscallEvent*>(ev);

    if (nullptr == sys_ev) {
        VanadisGetThreadStateResp* resp = dynamic_cast<VanadisGetThreadStateResp*>(ev);
        if ( nullptr == resp ) { 
            output->fatal(CALL_INFO, -1,
                      "Error - received an event in the OS, but cannot cast it to "
                      "a system-call event.\n");
        } else {
            VanadisForkSyscall* syscall = dynamic_cast<VanadisForkSyscall*>(m_coreInfoMap[ resp->getCore()].getSyscall( resp->getThread() ));
            assert( syscall ); 
            syscall->complete( resp );
            delete syscall;
        }
    } else {

        auto process = m_coreInfoMap.at(sys_ev->getCoreID()).getProcess( sys_ev->getThreadID() );

        handleIncomingSyscall( process, sys_ev, core_links[sys_ev->getCoreID()] );
    } 
}

void VanadisNodeOSComponent::pageFaultHandler( MMU_Lib::RequestID reqId, unsigned link, unsigned core, unsigned hwThread, 
                unsigned pid,  uint32_t vpn, uint32_t faultPerms, uint64_t instPtr, uint64_t memVirtAddr ) 
{
    output->verbose(CALL_INFO, 1, 0, "RequestID=%#" PRIx64 " link=%d pid=%d vpn=%d perms=%#x instPtr=%#" PRIx64 "\n", reqId, link, pid, vpn, faultPerms, instPtr ); 

    auto tmp = new PageFault( reqId, link, core, hwThread, pid, vpn, faultPerms, instPtr, memVirtAddr );
    m_pendingFault.push( tmp );
    if ( 1 == m_pendingFault.size() ) {
        pageFault( tmp );
    } else { 
        output->verbose(CALL_INFO, 1, 0, "queue page fault\n" ); 
    }
}

void VanadisNodeOSComponent::pageFaultFini( MMU_Lib::RequestID reqId, unsigned link, unsigned pid, uint32_t vpn, bool success )
{
    output->verbose(CALL_INFO, 1, 0,"link=%d pid=%d vpn=%d %#" PRIx32 "\n",link,pid,vpn, vpn << m_pageShift );
    m_mmu->faultHandled( reqId, link, pid, vpn, success );
    delete m_pendingFault.front();
    m_pendingFault.pop();
    if ( m_pendingFault.size() ) {
        auto tmp = m_pendingFault.front();
        pageFault( tmp );
    }
}

void VanadisNodeOSComponent::pageFault( PageFault *info )
{
    MMU_Lib::RequestID reqId = info->reqId;
    unsigned link = info->link;
    unsigned pid = info->pid;
    uint32_t vpn = info->vpn;
    uint32_t faultPerms = info->faultPerms;

    assert(pid > 0);
    auto thread = m_threadMap.at(pid);
    uint64_t virtAddr = vpn << m_pageShift; 

    // this is confusing because we have to virtAddrs one is the page virtaddr and the is the address of the memory req that faulted,
    // the full addres was added for debug, we should git rid of the VPN at some point. 
    output->verbose(CALL_INFO, 1, 0,"virtAddr=%#" PRIx64 " %c%c%c instPtr=%#" PRIx64 " virtMemAddr=%#" PRIx64 "\n",virtAddr, 
            faultPerms & 0x4 ? 'R' : '-',
            faultPerms & 0x2 ? 'W' : '-',
            faultPerms & 0x1 ? 'X' : '-',
            info->instPtr,
            info->memVirtAddr);

    auto region = thread->findMemRegion( virtAddr + 1 );

    if ( region ) { 
        // -1 indicates the vpn is not mapped to a physical page
        uint32_t pagePerms = m_mmu->getPerms( pid, vpn);

        // We got here because a TLB has to have an address resolved.
        // There are two cases that can happen:
        // 1) Read or Write from a data tlb
        // 2) Read with Execute from a inst tlb

        output->verbose(CALL_INFO, 1, 0,"found region %#" PRIx64 "-%#" PRIx64 "\n",region->addr,region->addr + region->length);

        output->verbose(CALL_INFO, 1, 0,"virtAddr=%#010" PRIx64 ": fault-perms %c%c%c, VM-perms %c%c%c pagePerms=%#x\n",virtAddr, 
            faultPerms & 0x4 ? 'R' : '-',
            faultPerms & 0x2 ? 'W' : '-',
            faultPerms & 0x1 ? 'X' : '-',
            region->perms & 0x4 ? 'R' : '-',
            region->perms & 0x2 ? 'W' : '-',
            region->perms & 0x1 ? 'X' : '-',
            pagePerms);

        // if the page is present the fault wants to write but the page doesn't have write, could be COW  
        if( pagePerms != -1 && faultPerms & 0x2 &&  0 == (pagePerms & 0x2 ) ) {
            OS::Page* newPage;
            int origPPN = m_mmu->virtToPhys(pid,vpn);
            output->verbose(CALL_INFO, 1, 0,"COW ppn of origin page %d\n",origPPN);

            // check if we can upgrade the permission for this page 
            if ( ! MMU_Lib::checkPerms( faultPerms, region->perms ) ) {
                output->verbose(CALL_INFO, 1, 0,"core %d, hwThread %d, instPtr %#" PRIx64 " caused page fault at address %#" PRIx64 "\n", 
                    info->core,info->hwThread,info->instPtr,info->memVirtAddr);
                pageFaultFini( reqId, link, pid, vpn, false );
                return;
            }

            try {
                newPage = allocPage( );
            } catch ( int err ) {
                output->fatal(CALL_INFO, -1, "Error: ran out of physical memory\n");
            }

            thread->mapVirtToPage( vpn, newPage );

            output->verbose(CALL_INFO, 1, 0,"origin ppn %d new ppn %d\n",origPPN, newPage->getPPN());
            // map this physical page into the MMU for this process with the regions permissions 
            m_mmu->map( thread->getpid(), vpn, newPage->getPPN(), m_pageSize, region->perms );

            auto callback = new Callback( [=]() {
                pageFaultFini( reqId, link, pid, vpn);
            });
            copyPage( origPPN << m_pageShift, newPage->getPPN() << m_pageShift, m_pageSize, callback );
            return;
        }

        // the fault is in a present region, check to see if the retions permissions satisfy the fault 
        if ( ! MMU_Lib::checkPerms( faultPerms, region->perms ) ) {
            output->verbose(CALL_INFO, 1, 0, "memory fault instPtr=%#" PRIx64 ", could not be satified for %#" PRIx64 ", no permission wantPerms=%#x havePerms=%#x\n", 
                    info->instPtr,virtAddr,faultPerms,region->perms);
            pageFaultFini( reqId, link, pid, vpn, false );
            return;
        }

        int pageTablePerms =  m_mmu->getPerms( pid, vpn );
        output->verbose(CALL_INFO, 1, 0,"vpn %d perms %#x\n",vpn,pageTablePerms);
        if ( pageTablePerms > -1 ) {
            if ( ! MMU_Lib::checkPerms( faultPerms, region->perms ) ) {
                output->verbose(CALL_INFO, 1, 0,"core %d, hwThread %d, instPtr %#" PRIx64 " caused page fault at address %#" PRIx64 "\n", 
                    info->core,info->hwThread,info->instPtr,info->memVirtAddr);
                pageFaultFini( reqId, link, pid, vpn, false );
                return;
            }
            output->verbose(CALL_INFO, 1, 0,"using existing page vpn=%d\n",vpn);
            pageFaultFini( reqId, link, pid, vpn );
            return;
        }

        OS::Page* page = nullptr;

        uint8_t* data = nullptr;
        // if this region has backing 
        if ( region->backing ) {
            // if this region is mapped to an ELF file, check to see if the physical page is cached
            if ( region->backing->elfInfo ) {
                page = checkPageCache( region->backing->elfInfo, vpn );
                if ( nullptr == page ) {
                    data = readElfPage( output, region->backing->elfInfo, vpn, m_pageSize );
                }  else {
                    output->verbose(CALL_INFO, 1, 0,"found elf page vpn %d -> ppn %d\n",vpn, page->getPPN());
                }
            } else {
                assert( region->backing->data.size());
                data = region->readData( vpn << m_pageShift, m_pageSize );
            }
        }

        // if there is no physical backing for this virtual page, get a page
        if ( nullptr == page ) {
            try {
                page = allocPage( );
            } catch ( int err ) {
                output->fatal(CALL_INFO, -1, "Error: ran out of physical memory\n");
            }
            output->verbose(CALL_INFO, 1, 0,"alloced physical page %d\n", page->getPPN() );

            thread->mapVirtToPage( vpn, page );
        } else {
            page->incRefCnt();
            output->verbose(CALL_INFO, 1, 0,"using exiting physical page %d\n",page->getPPN());
        }

        // map this physical page into the MMU for this process 
        m_mmu->map( thread->getpid(), vpn, page->getPPN(), m_pageSize, region->perms );
        
        // if there's elfInfo for this region is mapped to a file update the page cache 
        if ( region->backing && region->backing->elfInfo && 0 == region->name.compare("text") ) {
            if ( nullptr != data ) { 
                updatePageCache( region->backing->elfInfo, vpn, page );
            } else {
                output->verbose(CALL_INFO, 1, 0,"fault handled link=%d pid=%d vpn=%d %#" PRIx32 " ppn=%d\n",link,pid,vpn, vpn << m_pageShift,page->getPPN());
                pageFaultFini( reqId, link, pid, vpn );
                return;
            }
        }

        auto callback = new Callback( [=]() {
            pageFaultFini( reqId, link, pid, vpn );
        });

        if ( nullptr == data ) {
            output->verbose(CALL_INFO, 1, 0,"zero page\n");
            data = new uint8_t[m_pageSize];
            bzero( data, m_pageSize );
        }
        writePage( page->getPPN() << m_pageShift, data, m_pageSize, callback );
        
    } else {

        output->verbose(CALL_INFO, 1, 0,"core %d, hwThread %d, instPtr %#" PRIx64 " caused page fault at address %#" PRIx64 "\n", 
            info->core, info->hwThread, info->instPtr, info->memVirtAddr );
        pageFaultFini( reqId, link, pid, vpn, false );
    } 
}

bool VanadisNodeOSComponent::PageMemReadReq::handleResp( StandardMem::Request* ev ) {
    
    auto iter = reqMap.find( ev->getID() ); 
    assert ( iter != reqMap.end() );

    StandardMem::ReadResp* req = dynamic_cast<StandardMem::ReadResp*>(ev);
    assert( req );
    assert( req->size == 64 );
    //printf("PageMemReadReq::%s() %s\n",__func__,req->getString().c_str());
    memcpy( data + iter->second, req->data.data(), req->size );
    reqMap.erase( iter );

    delete ev;

    if ( offset < length ) {
        sendReq();
    }

    return reqMap.empty();
}

void VanadisNodeOSComponent::PageMemReadReq::sendReq() {

//    printf("%s() offset=%zu length=%zu\n",__func__,m_currentReqOffset,length);
    if ( m_currentReqOffset < length ) {
 //       printf("%s() addr %#lx\n",__func__,addr + offset);
        StandardMem::Request* req = new SST::Interfaces::StandardMem::Read( addr + m_currentReqOffset, 64 );
        //printf("%s() %s\n",__func__,req->getString().c_str());
        reqMap[req->getID()] = m_currentReqOffset;
        m_currentReqOffset += 64;
        mem_if->send(req);
    }
}

bool VanadisNodeOSComponent::PageMemWriteReq::handleResp( StandardMem::Request* ev ) {
  //  printf("PageMemWriteReq::%s()\n",__func__);
    auto iter = reqMap.find( ev->getID() ); 
    assert ( iter != reqMap.end() );
    reqMap.erase( iter );
    delete ev;
    if ( offset < length ) {
        sendReq();
    }
    return reqMap.empty();
}

void VanadisNodeOSComponent::PageMemWriteReq::sendReq() {
   // printf("%s()\n",__func__);
    if ( offset < length ) {
        std::vector< uint8_t > buffer( 64);  

        memcpy( buffer.data(), data + offset, buffer.size() );
        StandardMem::Request* req = new SST::Interfaces::StandardMem::Write( addr + offset, buffer.size(), buffer );
        //printf("PageMemWriteReq::%s() %s\n",__func__,req->getString().c_str());
        reqMap[req->getID()] = offset;
        offset += buffer.size();
        mem_if->send(req);
    }
}
