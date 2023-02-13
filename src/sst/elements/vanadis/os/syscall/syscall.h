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

using namespace SST::Interfaces;

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
        BlockMemoryHandler( VanadisSyscall* obj, SST::Output* out, uint64_t addr, std::vector<uint8_t>& data )
            : MemoryHandler(obj,out), m_addr(addr), m_data(data),  m_offset(0) {
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
    };

    class ReadMemoryHandler : public BlockMemoryHandler {
    public:
        ReadMemoryHandler( VanadisSyscall* obj, SST::Output* out, uint64_t addr, std::vector<uint8_t>& data )
            : BlockMemoryHandler(obj,out,addr,data) {}

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
                auto req =  new StandardMem::Read( physAddr, length, 0, virtAddr );
                obj->m_output->verbose(CALL_INFO, 16, 0, " %s\n",req->getString().c_str());
                return req;
            }
        } 
    };

    class WriteMemoryHandler : public BlockMemoryHandler {
    public:
        WriteMemoryHandler( VanadisSyscall* obj, SST::Output* out, uint64_t addr, std::vector<uint8_t>& data )
            : BlockMemoryHandler(obj,out,addr,data) {}

        void handle(StandardMem::WriteResp* req ) override {} 

        StandardMem::Request* generateMemReq() override {
            uint64_t length = calcLength();

            std::vector<uint8_t> payload( length );
            memcpy( payload.data(), m_data.data() + m_offset, length );
            
            auto physAddr = obj->virtToPhys( getAddress(), true ); 

            if ( -1 == physAddr ) {
                return nullptr;
            } else {
                m_offset += length; 
                return new StandardMem::Write( physAddr, payload.size(), payload);
            }
        } 
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
    std::string& getName()  { return m_name; }


    uint64_t virtToPhys( uint64_t virtAddr, bool isWrite ) {
        auto physAddr = m_process->virtToPhys( virtAddr );
        if ( physAddr == -1 ) {
            m_output->verbose(CALL_INFO, 16, 0,"physAddr not found for virtAddr=%#" PRIx64  "\n",virtAddr);
            m_pageFaultAddr = virtAddr; 
            m_pageFaultIsWrite = isWrite;
        } else {
            m_output->verbose(CALL_INFO, 16, 0,"virtAddr %#" PRIx64 " -> physAddr %#" PRIx64 "\n",virtAddr,physAddr);
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
    virtual void memReqIsDone() {};

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
    void readMemory( uint64_t addr, std::vector<uint8_t>& data );
    void writeMemory( uint64_t addr, std::vector<uint8_t>& data );

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
    m_output->verbose(CALL_INFO, 16, 0,"addr=%#" PRIx64 "\n",addr);
    assert(nullptr == m_memHandler);
    m_memHandler = new ReadStringHandler( this, m_output, addr, str );
}

inline void VanadisSyscall::readMemory( uint64_t addr, std::vector<uint8_t>& buffer ) {
    m_output->verbose(CALL_INFO, 16, 0,"addr=%#" PRIx64 " length=%zu\n", addr, buffer.size() );
    assert(nullptr == m_memHandler);
    m_memHandler = new ReadMemoryHandler( this, m_output, addr, buffer );
}

inline void VanadisSyscall::writeMemory( uint64_t addr, std::vector<uint8_t>& buffer ) {
    m_output->verbose(CALL_INFO, 16, 0,"addr=%#" PRIx64 " length=%zu\n", addr, buffer.size() );
    assert(nullptr == m_memHandler);
    m_memHandler = new WriteMemoryHandler( this, m_output, addr, buffer );
}

inline bool VanadisSyscall::handleMemRespBase( StandardMem::Request* req )
{
    m_output->verbose(CALL_INFO, 16, 0, "recv memory event (%s)\n", req->getString().c_str());
    m_output->verbose(CALL_INFO, 16, 0,"m_pendingMem.size() %zu\n",m_pendingMem.size());

    auto find_event = m_pendingMem.find(req->getID());

    assert( find_event != m_pendingMem.end() ); 
    m_pendingMem.erase(find_event);

    if ( m_memHandler ) {
        m_output->verbose(CALL_INFO, 16, 0,"call mem handler\n");
        req->handle(m_memHandler); 
        if ( m_memHandler->isDone() ) {
            m_output->verbose(CALL_INFO, 16, 0,"mem handler done\n");
            delete m_memHandler;
            m_memHandler = nullptr;
            memReqIsDone();
        }
    }

    if ( handleMemResp( req ) || ( m_complete && (m_pendingMem.size() == 0) )) {
        m_output->verbose(CALL_INFO, 16, 0,"done\n");
        return true;
    }

    return false;
}

}
}

#endif
