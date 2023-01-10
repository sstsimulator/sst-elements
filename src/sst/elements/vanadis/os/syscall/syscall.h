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
#include "os/resp/voscallresp.h"

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisSyscall {

    class MemoryHandler : public StandardMem::RequestHandler {
    public:
        MemoryHandler( SST::Output* out ) : StandardMem::RequestHandler(out) {}
        virtual ~MemoryHandler() {}
        virtual bool isDone() = 0;
        virtual StandardMem::Request* generateMemReq(OS::ProcessInfo* process) = 0;
    };

    class ReadStringHandler : public MemoryHandler {
    public:
        ReadStringHandler( SST::Output* out, uint64_t addr, std::string& str ) : MemoryHandler(out), m_addr(addr), m_str(str), m_isDone(false) {}

        void handle(StandardMem::ReadResp* req ) override {
            for (size_t i = 0; i < req->size; ++i) {
                m_str.push_back(req->data[i]);

                if (req->data[i] == '\0') {
                    m_isDone = true;
                    return;
                }
            }
        }

        StandardMem::Request* generateMemReq(OS::ProcessInfo* process) override { 
            uint64_t length;
            if ( m_str.empty() ) {
                length = vanadis_line_remainder( getAddress(), 64 );
            } else {
                length = 64;
            }
            //printf("%s() addr=%#" PRIx64 " firstReadLen=%zu\n",__func__,getAddress(), length);
            return new StandardMem::Read( process->virtToPhys( getAddress() ), length);
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
        BlockMemoryHandler( SST::Output* out, uint64_t addr, std::vector<uint8_t>& data ) : MemoryHandler(out), m_addr(addr), m_data(data),  m_offset(0) {
            //printf("%s() %zu\n",__func__,m_data.size());
        }

        virtual ~BlockMemoryHandler() {}

        bool isDone() { 
            //printf("%s() offset=%zu size=%zu\n",__func__,m_offset,m_data.size());
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
        ReadMemoryHandler( SST::Output* out, uint64_t addr, std::vector<uint8_t>& data ) : BlockMemoryHandler(out,addr,data) {}

        void handle(StandardMem::ReadResp* req ) override {
            memcpy( m_data.data() + m_offset, req->data.data(), req->size );
            m_offset += req->size; 
        }
        StandardMem::Request* generateMemReq(OS::ProcessInfo* process) override { 

            //printf("%s() %zu\n",__func__,m_data.size());
            uint64_t length = calcLength();

            //printf("%s() addr=%#" PRIx64 " read length=%zu\n",__func__,getAddress(), length);
            uint64_t addr = process->virtToPhys( getAddress() );
            StandardMem::Read* req = new StandardMem::Read( addr, length);
            //printf("%s() %#" PRIx64 " %#" PRIx64 " %s \n",__func__,getAddress(),addr,req->getString().c_str());
            return req;
        } 
    };

    class WriteMemoryHandler : public BlockMemoryHandler {
    public:
        WriteMemoryHandler( SST::Output* out, uint64_t addr, std::vector<uint8_t>& data ) : BlockMemoryHandler(out,addr,data) {}

        void handle(StandardMem::WriteResp* req ) override {
            //printf("WriteMemoryHandler::%s wrote %zu bytes\n",__func__,req->size);
        }

        StandardMem::Request* generateMemReq(OS::ProcessInfo* process) override {
            //printf("%s() %zu\n",__func__,m_data.size());
            uint64_t length = calcLength();

            //printf("%s() addr=%#" PRIx64 " [ %#" PRIx64 "] write length=%zu\n",__func__,getAddress(), process->virtToPhys( getAddress() ), length);

            std::vector<uint8_t> payload( length );
            memcpy( payload.data(), m_data.data() + m_offset, length );
            
            StandardMem::Request* req = new StandardMem::Write( process->virtToPhys( getAddress() ), payload.size(), payload);
            //printf( "send memory event (%s)\n", req->getString().c_str());

            // do this last because it is used by getAddress()
            m_offset += length; 
            return req;
        } 
    };


  public:
    typedef std::function<void(VanadisSyscall*, StandardMem::Request*)> SendMemReqFunc;

    VanadisSyscall( Output* output, Link* respLink, OS::ProcessInfo* process, SendMemReqFunc* func, VanadisSyscallEvent* event, std::string name )
        : m_output(output), m_respLink(respLink), m_process(process), m_memReqSend(func), 
            m_event(event), m_name(name), m_returnCode(0), m_success(true), m_complete(false), m_memHandler(nullptr), m_sendResp(true), m_hasExited(false) {}

    virtual ~VanadisSyscall() { 
        if ( m_sendResp ) {
            m_output->verbose(CALL_INFO, 16, 0,"pid=%d tid=%d syscall %s is complete\n",
                m_process->getpid(), m_process->gettid(), m_name.c_str() );
            VanadisSyscallResponse* resp = new VanadisSyscallResponse(m_returnCode,m_success,m_hasExited);;
            resp->setHWThread( m_event->getThreadID() );
            m_output->verbose(CALL_INFO, 16, 0,
                                "handler is completed and all memory events are "
                                "processed, sending response (= %" PRId64 " / 0x%llx, %s) to core.\n",
                                resp->getReturnCode(), resp->getReturnCode(),
                                resp->isSuccessful() ? "success" : "failed");

            sendResp(resp);
        } else {
            m_output->verbose(CALL_INFO, 16, 0,"syscall %s is complete\n", m_name.c_str() );
        }
        delete m_event;
    }

    virtual bool isComplete() { return m_complete && m_pendingMem.empty(); }

    bool handleMemRespBase( StandardMem::Request* req );

  protected:

    template <class Type>
    Type getEvent() { return static_cast<Type>(m_event); }           
    virtual bool handleMemResp( StandardMem::Request* req ) { delete req; return false; };
    virtual void memReqIsDone() {};

    void setReturnSuccess( int returnCode ) { 
        setReturnCode( returnCode, true );
        setComplete();
    }
    void setReturnFail( int returnCode ) { 
        setReturnCode( returnCode, false );
        setComplete();
    }
    void setReturnCode( int returnCode, bool success = true ) { 
        m_returnCode = returnCode;
        m_success = success; 
    }
    void setReturnExited() {
        m_hasExited = true;
        setComplete();
    }

    void sendMemRequest( StandardMem::Request* ev ) { 
        m_pendingMem.insert(ev->getID());
        (*m_memReqSend)( this, ev ); 
    }


    void sendResp( VanadisSyscallResponse* resp  ) { m_respLink->send(resp);}
    void readString( uint64_t addr, std::string& data );
    void readMemory( uint64_t addr, std::vector<uint8_t>& data );
    void writeMemory( uint64_t addr, std::vector<uint8_t>& data );
    Output*             m_output;
    OS::ProcessInfo*    m_process;
    Link*               m_respLink;
    VanadisSyscallEvent*    m_event;
    bool m_sendResp;

  private:

    std::string m_name;
    void setComplete() { m_complete = true; }
    
    SendMemReqFunc* m_memReqSend;

    bool m_complete;
    int m_returnCode;
    bool m_success;
    bool m_hasExited;

    std::unordered_set<StandardMem::Request::id_t> m_pendingMem;
    MemoryHandler* m_memHandler;

};


inline void VanadisSyscall::readString( uint64_t addr, std::string& str ) {
    m_output->verbose(CALL_INFO, 16, 0,"addr=%#" PRIx64 "\n",addr);
    m_memHandler = new ReadStringHandler( m_output, addr, str );
    sendMemRequest( m_memHandler->generateMemReq(m_process) );
}

inline void VanadisSyscall::readMemory( uint64_t addr, std::vector<uint8_t>& buffer ) {
    m_output->verbose(CALL_INFO, 16, 0,"addr=%#" PRIx64 " length=%zu\n", addr, buffer.size() );
    m_memHandler = new ReadMemoryHandler( m_output, addr, buffer );
    sendMemRequest( m_memHandler->generateMemReq(m_process) );
}

inline void VanadisSyscall::writeMemory( uint64_t addr, std::vector<uint8_t>& buffer ) {
    m_output->verbose(CALL_INFO, 16, 0,"addr=%#" PRIx64 " length=%zu\n", addr, buffer.size() );
    m_memHandler = new WriteMemoryHandler( m_output, addr, buffer );
    sendMemRequest( m_memHandler->generateMemReq(m_process) );
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
        } else {
            sendMemRequest( m_memHandler->generateMemReq(m_process) );
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
