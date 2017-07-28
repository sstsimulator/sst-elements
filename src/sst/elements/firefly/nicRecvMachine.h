// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


class RecvMachine {

        #include "nicShmemRecvMachine.h"

        class StreamBase {
          public:
            StreamBase(Output& output, RecvMachine& rm) : 
                m_dbg(output), m_rm(rm),m_recvEntry(NULL)
            {}
            virtual ~StreamBase() {
                m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"notify \n");
                if ( m_recvEntry ) {
                    m_recvEntry->notify( m_hdr.src_vNicId, m_src, m_tag, m_matched_len );
                    delete m_recvEntry;
                }
            }
            virtual void processPkt( FireflyNetworkEvent* ev ) {
                m_rm.state_move_0( ev, this );
            } 
            RecvEntryBase* getRecvEntry() { return m_recvEntry; }
            virtual size_t length() { return m_matched_len; }
          protected:
            Output&         m_dbg;
            RecvMachine&    m_rm;
            RecvEntryBase*  m_recvEntry;
            MsgHdr          m_hdr;
            int             m_src;
            int             m_tag;
            int             m_matched_len;
        };

        #include "nicMsgStream.h"
        #include "nicRdmaStream.h"
        #include "nicShmemStream.h"

      public:

        typedef std::function<void()> Callback;

        RecvMachine( Nic& nic, int vc, int numVnics, 
                int nodeId, int verboseLevel, int verboseMask,
                int rxMatchDelay, int hostReadDelay, 
                std::function<std::pair<Hermes::MemAddr,size_t>(uint64_t)> func ) :
            m_nic(nic), 
            m_vc(vc), 
            m_rxMatchDelay( rxMatchDelay ),
            m_hostReadDelay( hostReadDelay ),
            m_blockedCallback( NULL ),
            m_notifyCallback( false ), 
            m_shmem( m_dbg )
#ifdef NIC_RECV_DEBUG
            , m_msgCount(0) 
#endif
        { 
            char buffer[100];
            snprintf(buffer,100,"@t:%d:Nic::RecvMachine::@p():@l vc=%d ",nodeId,m_vc);

            m_dbg.init(buffer, verboseLevel, verboseMask, Output::STDOUT);
            m_shmem.init( func );
            setNotify();
        }

        virtual ~RecvMachine();

        void notify( ) {
            m_nic.schedCallback( [=](){ state_0( getNetworkEvent( m_vc ) ); } );
            m_notifyCallback = false;
        }

        void addDma() {
            m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n");
            if ( m_blockedCallback ) {
                m_nic.schedCallback( m_blockedCallback );
                m_blockedCallback = NULL; 
            }
        }

        void printStatus( Output& out );

        void setNotify( ) {
            assert( ! m_notifyCallback );
            m_nic.m_linkControl->setNotifyOnReceive(
                                    m_nic.m_recvNotifyFunctor );
            m_notifyCallback = true;
        }
        Nic& nic() { return m_nic; }

      protected:
        virtual void state_0( FireflyNetworkEvent* );
        void state_1( FireflyNetworkEvent* );
        void state_2( FireflyNetworkEvent* );
        void state_move_0( FireflyNetworkEvent*, StreamBase* );
        void state_move_1( FireflyNetworkEvent*, bool, StreamBase* );
        void state_move_2( FireflyNetworkEvent* event );
        void checkNetwork();

        void processNeedRecv( FireflyNetworkEvent* event, Callback callback ) {
            m_blockedCallback = callback;
            MsgHdr& hdr = *(MsgHdr*) event->bufPtr();
            m_nic.notifyNeedRecv( hdr.dst_vNicId, hdr.src_vNicId,
                     event->src, hdr.len);
        }

        FireflyNetworkEvent* getNetworkEvent(int vc ) {
            SST::Interfaces::SimpleNetwork::Request* req =
                m_nic.m_linkControl->recv(vc);
            if ( req ) {
                Event* payload = req->takePayload();
                if ( NULL == payload ) return NULL;
                FireflyNetworkEvent* event =
                    static_cast<FireflyNetworkEvent*>(payload);
                event->src = m_nic.NetToId( req->src );
                delete req;
                return event;
            } else {
                return NULL;
            }
        }

        Nic&        m_nic;
        Output      m_dbg;
        int         m_vc;

        int             m_rxMatchDelay;
        int             m_hostReadDelay;
        Callback        m_blockedCallback;
        bool            m_notifyCallback; 

        std::map< int, StreamBase* >    m_streamMap;

        Shmem           m_shmem;
#ifdef NIC_RECV_DEBUG 
        unsigned int    m_msgCount;
#endif
};

class CtlMsgRecvMachine : public RecvMachine {
  public:
    CtlMsgRecvMachine( Nic& nic, int vc, int numVnics, 
                int nodeId, int verboseLevel, int verboseMask,
                int rxMatchDelay, int hostReadDelay, 
                std::function<std::pair<Hermes::MemAddr,size_t>(uint64_t)> func ) :
        RecvMachine( nic, vc, numVnics, nodeId, verboseLevel, verboseMask, rxMatchDelay, hostReadDelay, func )
    {}

  private:
    void state_3( SendEntryBase* entry ) {
        m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n");
        m_nic.m_sendMachine[0]->run( entry );

        checkNetwork();
    } 

    void state_0( FireflyNetworkEvent* ev ) {

        MsgHdr& hdr = *(MsgHdr*) ev->bufPtr();
        RdmaMsgHdr& rdmaHdr = *(RdmaMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );

        m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"CtlMsg Get Operation src=%d len=%lu\n",ev->src,hdr.len);

        assert( hdr.op == MsgHdr::Rdma && rdmaHdr.op == RdmaMsgHdr::Get  );
        
        SendEntryBase* entry = m_nic.findGet( ev->src, hdr, rdmaHdr );
        assert(entry);

        ev->bufPop(sizeof(MsgHdr) + sizeof(rdmaHdr) );

        m_nic.schedCallback( 
                std::bind( &Nic::CtlMsgRecvMachine::state_3, *this, entry ),
                m_hostReadDelay );

        delete ev;
    }
};
