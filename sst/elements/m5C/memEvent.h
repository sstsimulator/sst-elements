// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _memEvent_h
#define _memEvent_h

#include <sst/core/component.h>

#include <mem/mem_object.hh>

#include <debug.h>

namespace SST {
namespace M5 {

class MemEvent : public SST::Event {

  public:
    enum Type {
        Timing,
        Functional
    };

    class Request {
        friend class MemEvent;

        // copied from m5 src/mem/request.h
        typedef uint8_t PrivateFlagsType;
        static const PrivateFlagsType VALID_SIZE           = 0x00000001;
        static const PrivateFlagsType VALID_PADDR          = 0x00000002;
        static const PrivateFlagsType VALID_VADDR          = 0x00000004;
        static const PrivateFlagsType VALID_PC             = 0x00000010;
        static const PrivateFlagsType VALID_CONTEXT_ID     = 0x00000020;
        static const PrivateFlagsType VALID_THREAD_ID      = 0x00000040;
        static const PrivateFlagsType VALID_EXTRA_DATA     = 0x00000080;

      public:
        Request( ::Request &req ) :
            m_privateFlags( 0 )
        {
            if ( req.hasPaddr() ) {
                m_paddr = req.getPaddr();
                m_privateFlags |= VALID_PADDR; 
                DBGX(4,"m_paddr=%#lx\n",m_paddr);
            }

            if ( req.hasSize() ) {
                m_size = req.getSize();
                m_privateFlags |= VALID_SIZE; 
                DBGX(4,"m_size=%lu\n",m_size);
            }

            m_flags = req.getFlags();
            DBGX(4,"m_flags=%#lx\n",m_flags);

            m_time = req.time();
            DBGX(4,"m_time=%lu\n",m_time);

            if ( ! req.hasPaddr() ) {
                m_asid = req.getAsid();
                m_vaddr = req.getVaddr();
                m_privateFlags |= VALID_VADDR; 
                DBGX(4,"m_asid=%#lx\n",m_asid);
                DBGX(4,"m_vaddr=%#lx\n",m_vaddr);
            }

            if ( req.extraDataValid() ) {
                m_extraData = req.getExtraData();
                m_privateFlags |= VALID_EXTRA_DATA; 
                DBGX(4,"m_extraData=%lu\n",m_extraData);
            }

            if ( req.hasContextId() ) {
                m_contextId = req.contextId();
                m_threadId = req.threadId();
                m_privateFlags |= VALID_CONTEXT_ID | VALID_THREAD_ID; 
                DBGX(4,"m_contextId=%lu\n",m_contextId);
                DBGX(4,"m_threadId=%lu\n",m_threadId);
            }

            if ( req.hasPC() ) {
                m_pc = req.getPC();
                m_privateFlags |= VALID_PC; 
                DBGX(4,"m_pc=%lu\n",m_pc);
            }
        }

        ::Request* M5_Request()
        {
            if ( m_privateFlags & VALID_VADDR ) {
                DBGX(4,"new Request vaddr=%#lx\n",m_vaddr);
                return new ::Request( m_asid, m_vaddr, m_size, m_flags,
                                        m_pc, m_contextId, m_threadId );
            } else {
                DBGX(4,"new Request paddr=%#lx\n",m_paddr);
                return  new ::Request( m_paddr, m_size, m_flags );
            }
        }

      private:
        Request() {}

        Addr        m_paddr;
        int         m_size;
        uint32_t    m_flags;
        uint64_t    m_privateFlags;
        uint64_t    m_time;
        int         m_asid;
        Addr        m_vaddr;
        uint64_t    m_extraData;
        int         m_contextId;
        int         m_threadId;
        Addr        m_pc;

        friend class boost::serialization::access;
        template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_NVP(m_paddr);
            ar & BOOST_SERIALIZATION_NVP(m_size);
            ar & BOOST_SERIALIZATION_NVP(m_flags);
            ar & BOOST_SERIALIZATION_NVP(m_privateFlags);
            ar & BOOST_SERIALIZATION_NVP(m_time);
            ar & BOOST_SERIALIZATION_NVP(m_asid);
            ar & BOOST_SERIALIZATION_NVP(m_vaddr);
            ar & BOOST_SERIALIZATION_NVP(m_extraData);
            ar & BOOST_SERIALIZATION_NVP(m_contextId);
            ar & BOOST_SERIALIZATION_NVP(m_threadId);
            ar & BOOST_SERIALIZATION_NVP(m_pc);
        }
    };

  private:
    // copied from m5 src/mem/packet.h
    typedef uint32_t FlagsType;
    static const FlagsType VALID_ADDR             = 0x00000100;
    static const FlagsType VALID_SIZE             = 0x00000200;
    static const FlagsType VALID_SRC              = 0x00000400;
    static const FlagsType VALID_DST              = 0x00000800;
    static const FlagsType STATIC_DATA            = 0x00001000;
    static const FlagsType DYNAMIC_DATA           = 0x00002000;
    static const FlagsType ARRAY_DATA             = 0x00004000;

  public:

    MemEvent( PacketPtr pkt, Type type = Timing ) :
        m_type( type ),
        m_isResponse( false ),
        m_flags( pkt->getFlags() ),
        m_cmd( pkt->cmd.toInt() ),
        m_req( *pkt->req ),
        m_senderState( (uint64_t) pkt )
    {
        DBGX(4,"pkt=%p %s cmd=`%s` %s addr=%#x\n",
                pkt, type == Timing ? "Timing" : "Functional", 
                pkt->cmdString().c_str(), 
                pkt->isRequest() ? (pkt->needsResponse() ? "needs" :""):"",
                pkt->getAddr() );

        setPriority( MEMEVENTPRIORITY );

        if ( m_flags & VALID_ADDR ) {
            m_addr = pkt->getAddr();
            DBGX(4,"m_addr=%#lx\n",m_addr);
        }

        if ( m_flags & VALID_SIZE ) {
            m_size = pkt->getSize();
            DBGX(4,"m_size=%#x\n",m_size);
        }

        if ( m_flags & VALID_SRC ) {
            m_src = pkt->getSrc();
            DBGX(4,"m_src=%#x\n",m_src);
        }

        if ( m_flags & VALID_DST ) {
            m_dest = pkt->getDest();
            DBGX(4,"m_dest=%#x\n",m_dest);
        }

        if ( pkt->hasData() ) {
            DBGX(4,"hasData\n");
            m_data.resize( pkt->getSize() );
            pkt->writeData( &m_data[0] );
        }

        if ( pkt->isResponse() ) {
            DBGX(4,"isResponse\n");
            m_isResponse = true;
            m_senderState =  (uint64_t) pkt->senderState;
            delete pkt;
        }
	// should we delete the packet if this is a request but we
	// don't expect a response
    }

    Type        type() { return m_type; }

    PacketPtr M5_Packet() {
        PacketPtr pkt;

        if (  m_isResponse ) {
            pkt = (PacketPtr) m_senderState;
            if ( ! pkt->memInhibitAsserted() ) {
            	pkt->makeResponse();
            }
            DBGX(4,"isResponse pkt=%p\n",m_senderState);
        } else {
            ::Request* req = m_req.M5_Request();
            if ( req->getPaddr() == m_addr && (unsigned)req->getSize() == m_size ) {
                pkt = new ::Packet( req, MemCmd(m_cmd), m_dest );
            } else {
                pkt = new ::Packet( req, MemCmd(m_cmd), m_dest, m_size );
            }
            pkt->allocate();
            pkt->setSrc( m_src );
            pkt->senderState = (Packet::SenderState*) m_senderState;

            DBGX(4,"! isResponse\n");
        }

        if ( ! pkt->memInhibitAsserted() && pkt->hasData() ) {
            assert( m_data.size() == pkt->getSize() );
            DBGX(4,"hasData\n");
            pkt->setData( &m_data[0] );
        }

        DBGX(4,"pkt=%p %s cmd=`%s` %s addr=%#x\n",
                pkt, type() == Timing ? "Timing" : "Functional", 
                pkt->cmdString().c_str(), 
                pkt->isRequest() ? (pkt->needsResponse() ? "needs" :""):"",
                pkt->getAddr() );

        return pkt;
    }

  private:
    Type                    m_type;
    bool                    m_isResponse;
    FlagsType               m_flags; 

    int                     m_cmd;
    Request                 m_req;
    std::vector<uint8_t>    m_data;
    Addr                    m_addr;
    unsigned                m_size;
    Packet::NodeID          m_src;
    Packet::NodeID          m_dest;
    uint64_t                m_senderState;

  private:

    MemEvent() {}

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::Event);

        ar & BOOST_SERIALIZATION_NVP(m_type);
        ar & BOOST_SERIALIZATION_NVP(m_isResponse);
        ar & BOOST_SERIALIZATION_NVP(m_flags);

        ar & BOOST_SERIALIZATION_NVP(m_cmd);
        ar & BOOST_SERIALIZATION_NVP(m_req);
        ar & BOOST_SERIALIZATION_NVP(m_data);
        ar & BOOST_SERIALIZATION_NVP(m_size);
        ar & BOOST_SERIALIZATION_NVP(m_src);
        ar & BOOST_SERIALIZATION_NVP(m_dest);
        ar & BOOST_SERIALIZATION_NVP(m_addr);
        ar & BOOST_SERIALIZATION_NVP(m_senderState);
    }
};

}
}

#endif
