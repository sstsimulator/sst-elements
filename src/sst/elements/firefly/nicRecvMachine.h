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

        
    typedef uint64_t SrcKey;
    typedef std::function<void()> Callback;
    static SrcKey getSrcKey(int srcNode, int srcPid) { return (SrcKey) srcNode << 32 | srcPid; }

    #include "nicRecvStream.h"
    #include "nicRecvCtx.h"
    #include "nicMsgStream.h"
    #include "nicRdmaStream.h"
    #include "nicShmemStream.h"

      public:

        RecvMachine( Nic& nic, int vc, int numVnics, 
                int nodeId, int verboseLevel, int verboseMask,
                int rxMatchDelay, int hostReadDelay, int maxQsize ) :
            m_nic(nic), 
            m_vc(vc), 
            m_rxMatchDelay( rxMatchDelay ),
            m_hostReadDelay( hostReadDelay ),
            m_notifyCallback( false )
        { 
            char buffer[100];
            snprintf(buffer,100,"@t:%d:Nic::RecvMachine::@p():@l vc=%d ",nodeId,m_vc);

            m_dbg.init(buffer, verboseLevel, verboseMask, Output::STDOUT);
            setNotify();
            for ( unsigned i=0; i < numVnics; i++) {
                m_ctxMap.push_back( new Ctx( m_dbg, *this, i, maxQsize ) );
            }
        }

        virtual ~RecvMachine(){}

        void regMemRgn( int pid, int rgnNum, MemRgnEntry* entry ) {
            m_ctxMap[pid]->regMemRgn( rgnNum, entry );
        } 

        void regGetOrigin( int pid, int key, DmaRecvEntry* entry ) {
            m_ctxMap[pid]->regGetOrigin( key, entry );
        }
        void postRecv( int pid, DmaRecvEntry* entry ) {
            m_ctxMap[pid]->postRecv( entry );
        }    
        Nic& nic() { return m_nic; }
        void printStatus( Output& out );

    protected:
        Nic&        m_nic;
        Output      m_dbg;
        int         m_hostReadDelay;
        std::vector< Ctx* >   m_ctxMap;

        void checkNetworkForData() {
            FireflyNetworkEvent* ev = getNetworkEvent( m_vc );
            if ( ev ) {
                m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"packet available\n");
                m_nic.schedCallback( std::bind( &Nic::RecvMachine::processPkt, this, ev ), 1);
            } else {
                m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"network idle\n");
                setNotify();
            }
        }
        
	private:
        void setNotify( ) {
            assert( ! m_notifyCallback );
            if( ! m_notifyCallback ) {
                m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "\n");
                m_nic.m_linkWidget.setNotifyOnReceive(
                                    std::bind(&Nic::RecvMachine::processNetworkData, this), m_vc );
                m_notifyCallback = true;
            }
        }

        void processNetworkData() {
            m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "\n");

            // this notifier was called by the LinkControl object, the RecvMachine may 
            // re-install the LinkControl notifier, if it does there would be a cycle
            // this schedCallback breaks the cycle
            m_nic.schedCallback(  [=](){ processPkt( getNetworkEvent( m_vc ) ); } );
            m_notifyCallback = false;
        }

        virtual void processPkt( FireflyNetworkEvent* ev ) {
            // if the event was consumed, we can can check for the next
            if ( ! m_ctxMap[ ev->getDestPid() ]->processPkt( ev ) ) {
                checkNetworkForData();
            } else {
      		    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"blocked\n");
            }
        }

        FireflyNetworkEvent* getNetworkEvent(int vc ) {
            SST::Interfaces::SimpleNetwork::Request* req =
                m_nic.m_linkControl->recv(vc);
            if ( req ) {
                Event* payload = req->takePayload();
                if ( NULL == payload ) return NULL;
                FireflyNetworkEvent* event =
                    static_cast<FireflyNetworkEvent*>(payload);
                event->setSrcNode( m_nic.NetToId( req->src ) );
                delete req;
                return event;
            } else {
                return NULL;
            }
        }

        int         m_vc;
        int         m_unit;
        int         m_rxMatchDelay;
        bool        m_notifyCallback; 

};

class CtlMsgRecvMachine : public RecvMachine {
  public:
    CtlMsgRecvMachine( Nic& nic, int vc, int numVnics, 
                int nodeId, int verboseLevel, int verboseMask,
                int rxMatchDelay, int hostReadDelay, int maxQsize ) :
        RecvMachine( nic, vc, numVnics, nodeId, verboseLevel, verboseMask, rxMatchDelay, hostReadDelay, maxQsize )
    {}

  private:
    void queueResponse( SendEntryBase* entry ) {
        m_nic.m_sendMachine[0]->run( entry );
        checkNetworkForData();
    } 

    void processPkt( FireflyNetworkEvent* ev ) {

        MsgHdr& hdr = *(MsgHdr*) ev->bufPtr();
        RdmaMsgHdr& rdmaHdr = *(RdmaMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );

        m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"CtlMsg Get Operation srcNode=%d op=%d rgn=%d resp=%d, offset=%d\n",
                ev->getSrcNode(), rdmaHdr.op, rdmaHdr.rgnNum, rdmaHdr.respKey, rdmaHdr.offset );

        assert( hdr.op == MsgHdr::Rdma && rdmaHdr.op == RdmaMsgHdr::Get  );
        
        SendEntryBase* entry = m_ctxMap[ev->getDestPid()]->findGet( ev->getSrcNode(), ev->getSrcPid(), rdmaHdr );
        assert(entry);

        ev->bufPop(sizeof(MsgHdr) + sizeof(rdmaHdr) );

        m_nic.schedCallback( 
                std::bind( &Nic::CtlMsgRecvMachine::queueResponse, this, entry ),
                m_hostReadDelay );

        delete ev;
    }
};
