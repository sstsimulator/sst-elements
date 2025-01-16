// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OS_SYSCALL_SYSCALL
#define _H_VANADIS_OS_SYSCALL_SYSCALL

#include <sst/core/link.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/output.h>
#include "util/vlinesplit.h"
#include "os/voscallev.h"
#include "os/include/process.h"
#include "os/vgetthreadstate.h"
#include "os/resp/voscallresp.h"
#include "os/vosDbgFlags.h"

using namespace SST::Interfaces;

#define LINUX_EBADF            9
#define LINUX_EAGAIN          11
#define LINUX_EINVAL          22
#define LINUX_ENOTTY          25

namespace SST {
namespace Vanadis {

class VanadisNodeOSComponent;

class VanadisSyscall {

private:

    class MemoryHandler : public StandardMem::RequestHandler {
    public:
        MemoryHandler( VanadisSyscall* obj, SST::Output* out )
            : StandardMem::RequestHandler(out), obj(obj) {}
        virtual ~MemoryHandler() {}
        virtual bool isDone() = 0;
        virtual StandardMem::Request* generateMemReq() = 0;

        VanadisSyscall* obj;
    };

    class ReadStringHandler : public MemoryHandler {
    public:
        ReadStringHandler( VanadisSyscall* obj, SST::Output* out, uint64_t addr, std::string& str )
            : MemoryHandler(obj,out), m_addr(addr), m_str(str), m_isDone(false) {}

        void handle(StandardMem::ReadResp* req ) override {
            for (size_t i = 0; i < req->size; ++i) {
                m_str.push_back(req->data[i]);

                if (req->data[i] == '\0') {
                    m_isDone = true;
                    return;
                }
            }
        }

        StandardMem::Request* generateMemReq() override {
            uint64_t length;
            if ( m_str.empty() ) {
                
                length = vanadis_line_remainder( getAddress(), 64 );
                
            } else {
                
                length = 64;
                
            }
            auto physAddr = obj->virtToPhys( getAddress(), false );
            
            if ( -1 == physAddr ) {
                
                return nullptr;
            } else {
                
                return new StandardMem::Read( physAddr, length);
            }
        } 

        bool isDone() override { return m_isDone; }

    private:
        uint64_t getAddress() { return m_addr + m_str.length(); };        
        bool m_isDone;
        std::string& m_str;
        uint64_t m_addr;
    };

    class BlockMemoryHandler : public MemoryHandler {
    public:
        BlockMemoryHandler( VanadisSyscall* obj, SST::Output* out, uint64_t addr, std::vector<uint8_t>& data, bool lock )
            : MemoryHandler(obj,out), m_addr(addr), m_data(data),  m_offset(0), m_lock(lock) {
        }

        virtual ~BlockMemoryHandler() {}

        bool isDone() { 
            return m_offset == m_data.size(); 
        }

    protected:
        size_t calcLength() {
            uint64_t length;
            if ( 0 == m_offset ) {
                length = vanadis_line_remainder( getAddress(), 64 );
            } else {
                length = 64;
            }
            length = m_data.size() - m_offset < length ? m_data.size() - m_offset : length;
            return length;
        }
        uint64_t getAddress() { return m_addr + m_offset; };        
        std::vector<uint8_t>& m_data;
        uint64_t m_addr;
        size_t m_offset;
        int m_lock;
    };

    class ReadMemoryHandler : public BlockMemoryHandler {
    public:
        ReadMemoryHandler( VanadisSyscall* obj, SST::Output* out, uint64_t addr, std::vector<uint8_t>& data, bool lock )
            : BlockMemoryHandler(obj,out,addr,data,lock) {}

        void handle(StandardMem::ReadResp* req ) override {
            memcpy( m_data.data() + m_offset, req->data.data(), req->size );
            m_offset += req->size; 
        }
        StandardMem::Request* generateMemReq() override {

            auto length = calcLength();

            auto virtAddr = getAddress();

            auto physAddr = obj->virtToPhys( virtAddr, false );

            if ( -1 == physAddr ) {

                return nullptr;
            } else {

                if ( m_lock ) {

                    auto req =  new StandardMem::LoadLink( physAddr, length, 0, virtAddr, 0, 0 );
                    
                    obj->m_output->verbose(CALL_INFO, 16, 0, " %s\n",req->getString().c_str());
                    return req;
                } else {

                    auto req =  new StandardMem::Read( physAddr, length, 0, virtAddr, 0, 0 );
                    obj->m_output->verbose(CALL_INFO, 16, 0, " %s\n",req->getString().c_str());
                    return req;
                }
            }
        } 
    };

    class WriteMemoryHandler : public BlockMemoryHandler {
    public:
        WriteMemoryHandler( VanadisSyscall* obj, SST::Output* out, uint64_t addr, std::vector<uint8_t>& data, bool lock, OS::ProcessInfo* process  = nullptr)
            : BlockMemoryHandler(obj,out,addr,data,lock), process( process ) {}

        void handle(StandardMem::WriteResp* req ) override {
            m_offset += req->size;
        }

        StandardMem::Request* generateMemReq() override {

            uint64_t length = calcLength();

            std::vector<uint8_t> payload( length );
            memcpy( payload.data(), m_data.data() + m_offset, length );
            
            auto physAddr = obj->virtToPhys( getAddress(), true, process ); 

            if ( -1 == physAddr ) {
                return nullptr;
            } else {
                if ( m_lock ) {
                    return new StandardMem::StoreConditional( physAddr, payload.size(), payload, 0);
                } else {
                    return new StandardMem::Write( physAddr, payload.size(), payload, 0);
                }
            }
        } 
    private:
        OS::ProcessInfo* process;
    };


  public:

    struct ReturnInfo {
        ReturnInfo( ) : code(0), success(true), hasExited(false) {}
        int code;
        bool success;
        bool hasExited;
    };

    VanadisSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallEvent* event, std::string name );
    virtual ~VanadisSyscall();

    virtual void handleEvent( VanadisCoreEvent* ) { assert(0); } 
    virtual bool isComplete() { return m_complete && m_pendingMem.empty(); }
    bool handleMemRespBase( StandardMem::Request* req );
    int getCoreId()         { return m_process->getCore(); }
    int getThreadId()       { return m_process->getHwThread(); }
    int getPid()            { return m_process->getpid(); }
    int getTid()            { return m_process->gettid(); }
    std::string& getName()  { return m_name; }


    uint64_t virtToPhys( uint64_t virtAddr, bool isWrite, OS::ProcessInfo* process = nullptr ) {
        uint64_t physAddr = m_process->virtToPhys( virtAddr );
        if ( process ) {
            physAddr = process->virtToPhys( virtAddr );
        } else {
            physAddr = m_process->virtToPhys( virtAddr );
        }
        if ( physAddr == -1 ) {
            m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"physAddr not found for virtAddr=%#" PRIx64  "\n",virtAddr);
            m_pageFaultAddr = virtAddr; 
            m_pageFaultIsWrite = isWrite;
        } else {
            m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"virtAddr %#" PRIx64 " -> physAddr %#" PRIx64 "\n",virtAddr,physAddr);
        }
        return physAddr;
    } 

    StandardMem::Request* getMemoryRequest() {
        m_output->verbose(CALL_INFO, 16, 0,"\n");
        StandardMem::Request* req = nullptr;
        if ( m_memHandler ) {

            req = m_memHandler->generateMemReq();

            if ( req ) {

                m_pendingMem.insert(req->getID());

            }

        }

        return req;
    }

    bool causedPageFault() {
        return (m_pageFaultAddr);
    }

    std::tuple<uint64_t,bool> getPageFault() {
        return std::make_tuple(m_pageFaultAddr,m_pageFaultIsWrite);
    }

  protected:

    template <class Type>
    Type getEvent() { return static_cast<Type>(m_event); }           
    virtual bool handleMemResp( StandardMem::Request* req ) { delete req; return false; };
    virtual void memReqIsDone( bool ) {};

    void sendNeedThreadState() { 
        m_coreLink->send( new VanadisGetThreadStateReq( m_event->getThreadID() ) );
    }

    void setReturnSuccess( int returnCode ) { 
        setReturnCode( returnCode, true );
        setComplete();
    }
    void setReturnFail( int returnCode ) { 
        setReturnCode( returnCode, false );
        setComplete();
    }
    void setReturnCode( int returnCode, bool success = true ) { 
        m_returnInfo.code = returnCode;
        m_returnInfo.success = success;
    }
    void setReturnExited() {
        m_returnInfo.hasExited = true;
        setComplete();
    }

    void readString( uint64_t addr, std::string& data );
    void readMemory( uint64_t addr, std::vector<uint8_t>& data, bool lock = false );
    void writeMemory( uint64_t addr, std::vector<uint8_t>& data, bool lock = false );
    void writeMemory( uint64_t addr, std::vector<uint8_t>& data, OS::ProcessInfo* process );

    Output*                 m_output;
    OS::ProcessInfo*        m_process;
    VanadisSyscallEvent*    m_event;
    VanadisNodeOSComponent* m_os;

  private:

    void setComplete() { m_complete = true; }

    bool    m_complete;

    uint64_t            m_pageFaultAddr;
    uint64_t            m_pageFaultIsWrite;
    std::string         m_name;
    ReturnInfo          m_returnInfo;
    MemoryHandler*      m_memHandler;

    std::unordered_set<StandardMem::Request::id_t> m_pendingMem;

    SST::Link*          m_coreLink;
};

inline void VanadisSyscall::readString( uint64_t addr, std::string& str ) {
    m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"addr=%#" PRIx64 "\n",addr);
    assert(nullptr == m_memHandler);
    m_memHandler = new ReadStringHandler( this, m_output, addr, str );
}

inline void VanadisSyscall::readMemory( uint64_t addr, std::vector<uint8_t>& buffer, bool lock ) {
    m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"addr=%#" PRIx64 " length=%zu\n", addr, buffer.size() );
    assert(nullptr == m_memHandler);
    m_memHandler = new ReadMemoryHandler( this, m_output, addr, buffer, lock );
}

inline void VanadisSyscall::writeMemory( uint64_t addr, std::vector<uint8_t>& buffer, OS::ProcessInfo* process ) {
    m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"writeMemory addr=%#" PRIx64 " length=%zu\n", addr, buffer.size() );
    assert(nullptr == m_memHandler);
    m_memHandler = new WriteMemoryHandler( this, m_output, addr, buffer, false, process );
}

inline void VanadisSyscall::writeMemory( uint64_t addr, std::vector<uint8_t>& buffer, bool lock ) {
    m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"writeMemory addr=%#" PRIx64 " length=%zu\n", addr, buffer.size() );
    assert(nullptr == m_memHandler);
    m_memHandler = new WriteMemoryHandler( this, m_output, addr, buffer, lock );
}

inline bool VanadisSyscall::handleMemRespBase( StandardMem::Request* req )
{
    m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL, "recv memory event (%s)\n", req->getString().c_str());
    m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL,"m_pendingMem.size() %zu\n",m_pendingMem.size());

    auto find_event = m_pendingMem.find(req->getID());

    assert( find_event != m_pendingMem.end() ); 
    m_pendingMem.erase(find_event);

    if ( m_memHandler ) {

        req->handle(m_memHandler); 
        if ( m_memHandler->isDone() ) {

            delete m_memHandler;
            m_memHandler = nullptr;
            memReqIsDone( req->getFail() );
        }
    }

    if ( handleMemResp( req ) || ( m_complete && (m_pendingMem.size() == 0) )) {
        return true;
    }

    return false;
}

}
}

#endif
