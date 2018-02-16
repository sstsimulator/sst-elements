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

            SrcKey getSrcKey(int srcNode, int srcPid) { return (SrcKey) srcNode << 32 | srcPid; }

        class StreamBase {
            std::string m_prefix;
            const char* prefix() { return m_prefix.c_str(); }
          public:
            StreamBase(Output& output, RecvMachine& rm, int unit, int srcNode, int srcPid, int myPid ) : 
                m_dbg(output), m_rm(rm), m_unit(unit), m_srcNode(srcNode), m_srcPid(srcPid), m_myPid( myPid ),
                m_recvEntry(NULL),m_sendEntry(NULL), m_numPending(0), m_pktNum(0), m_expectedPkt(0), m_blockedNeedRecv(NULL)
            {
                m_prefix = "@t:"+ std::to_string(rm.nic().getNodeId()) +":Nic::RecvMachine::StreamBase::@p():@l ";
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"this=%p\n",this);
                m_start = m_rm.nic().getCurrentSimTimeNano();
				assert( unit >= 0 );
            }

            SrcKey getSrcKey() { return (SrcKey) m_srcNode << 32 | m_srcPid; }

            virtual ~StreamBase() {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"this=%p latency=%" PRIu64 "\n",this,
                                            m_rm.nic().getCurrentSimTimeNano()-m_start);
                if ( m_recvEntry ) {
                    m_recvEntry->notify( m_srcPid, m_srcNode, m_matched_tag, m_matched_len );
                    delete m_recvEntry;
                }
				if ( m_sendEntry ) {
					m_rm.m_nic.m_sendMachine[0]->run( m_sendEntry );	
				}
            }

            virtual bool postedRecv( DmaRecvEntry* entry );
            virtual void processPkt( FireflyNetworkEvent* ev );

            RecvEntryBase* getRecvEntry()   { return m_recvEntry; }
            virtual size_t length()         { return m_matched_len; }

            void setWakeup( Callback callback )    { 
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"\n");
                m_wakeupCallback = callback; 
            }

            bool isBlocked()    { 
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"%d\n",m_numPending == m_rm.getMaxQsize());
                return m_numPending == m_rm.getMaxQsize();
            }
            int getSrcPid()     { return m_srcPid; }
            int getSrcNode()    { return m_srcNode; }
            int getMyPid()      { return m_myPid; }
            int getUnit()       { return m_unit; }		


            void needRecv( FireflyNetworkEvent* ev ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"\n");
                m_blockedNeedRecv = ev;
                m_rm.needRecv( this );
            }

          protected:

            void ready( Callback callback, uint64_t pktNum );

            FireflyNetworkEvent* m_blockedNeedRecv;

            Callback        m_wakeupCallback;
            Output&         m_dbg;
            RecvMachine&    m_rm;
			SendEntryBase*  m_sendEntry;
            RecvEntryBase*  m_recvEntry;
            MsgHdr          m_hdr;
            int             m_srcNode;
            int             m_srcPid;
            int             m_myPid;
            int             m_matched_len;
            int             m_matched_tag;
            int             m_unit;
            SimTime_t       m_start;
            int             m_numPending;
            int             m_maxQsize;
            uint64_t        m_pktNum;
            uint64_t        m_expectedPkt;
        };


        class ProcessCtx {
        };

        #include "nicMsgStream.h"
        #include "nicRdmaStream.h"
        #include "nicShmemStream.h"

      public:

        RecvMachine( Nic& nic, int vc, int numVnics, 
                int nodeId, int verboseLevel, int verboseMask,
                int rxMatchDelay, int hostReadDelay ) :
            m_nic(nic), 
            m_vc(vc), 
            m_rxMatchDelay( rxMatchDelay ),
            m_hostReadDelay( hostReadDelay ),
            m_blockedNetworkEvent( NULL ), 
            m_notifyCallback( false ),
            m_streamMap(numVnics),
            m_postedRecvs(numVnics),
            m_memRgnM(numVnics),
            m_getOrgnM(numVnics),
            m_blockedNetworkEvents(numVnics,NULL)
#ifdef NIC_RECV_DEBUG
            , m_msgCount(0) 
#endif
        { 
            char buffer[100];
            snprintf(buffer,100,"@t:%d:Nic::RecvMachine::@p():@l vc=%d ",nodeId,m_vc);

            m_dbg.init(buffer, verboseLevel, verboseMask, Output::STDOUT);
            setNotify();
        }

        int getMaxQsize() { return 16; }

        virtual ~RecvMachine(){}

        DmaRecvEntry* findPut( int destPid, int src, MsgHdr& hdr, RdmaMsgHdr& rdmahdr );
        EntryBase* findRecv( int destPid, int srcNode, int srcPid, MsgHdr& hdr, MatchMsgHdr& matchHdr  );
        SendEntryBase* findGet( int destPid, int srcNode, int srcPid, RdmaMsgHdr& rdmaHdr );

        void regMemRgn( int pid, int rgnNum, MemRgnEntry* entry ) {
            m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "pid=%d rgnNum=%d\n",pid,rgnNum);
            m_memRgnM[pid][ rgnNum ] = entry; 
        } 

        void regGetOrigin( int pid, int key, DmaRecvEntry* entry ) {
            m_getOrgnM[pid][ key ] = entry; 
        }

        void postRecv( int pid, DmaRecvEntry* entry ) {

            // check to see if there are active streams for this pid
            // if there are they may be blocked waiting for the host to post a recv
            if ( ! m_streamMap[pid].empty() ) {
                 std::map< SrcKey, StreamBase* >& tmp = m_streamMap[ pid ];
                 std::map< SrcKey, StreamBase* >::iterator iter = tmp.begin();
                 for ( ; iter != tmp.end(); ++iter ) {
                    if( iter->second->postedRecv( entry ) ) {
                        return;
                    }       
                 }
            }
            m_postedRecvs[pid].push_back( entry );
        } 
 

        void setNotify( ) {
            //assert( ! m_notifyCallback );
            if( ! m_notifyCallback ) {
                m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "\n");
                m_nic.m_linkWidget.setNotifyOnReceive(
                                    std::bind(&Nic::RecvMachine::processNetworkData, this), m_vc );
                m_notifyCallback = true;
            }
        }
        Nic& nic() { return m_nic; }
        void printStatus( Output& out );

	private:
        void processNetworkData() {
            m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "\n");

            // this notifier was called by the LinkControl object, the RecvMachine may 
            // re-install the LinkControl notifier, if it does there would be a cycle
            // this schedCallback breaks the cycle
            m_nic.schedCallback(  [=](){ processPkt( getNetworkEvent( m_vc ) ); } );
            m_notifyCallback = false;
        }

      protected:
        void wakeup( StreamBase* stream, FireflyNetworkEvent* ev ) {
      		m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"\n");
            stream->processPkt( ev );
            checkNetworkForData();
        }
        virtual void processPkt( FireflyNetworkEvent* );

        StreamBase* newStream( int unit, FireflyNetworkEvent* );

        void clearMapAndDeleteStream( int src, StreamBase* stream ) {
      		m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"\n");
            clearStreamMap(src,stream->getSrcKey() );
            deleteStream(stream);
        }
        void deleteStream( StreamBase* stream ) {
      		m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"\n");
            freeNicUnit(stream->getUnit());
            delete stream;
        }

        void freeNicUnit( int unit ) {
            m_nic.freeNicUnit( unit );
            if ( m_blockedNetworkEvent ) {
                FireflyNetworkEvent* ev = m_blockedNetworkEvent;
                m_blockedNetworkEvent = NULL;
                processPkt( ev );
            }
        }

        void clearStreamMap( int pid, SrcKey key ) {
      		m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"pid=%d\n",pid);

            m_streamMap[pid].erase(key);
            if ( m_blockedNetworkEvents[pid] ) {
                FireflyNetworkEvent* ev = m_blockedNetworkEvents[pid];
                if(  getSrcKey( ev->getSrcNode(), ev->getSrcPid() ) == key ) {
                    m_blockedNetworkEvents[pid] = NULL;
      		        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"wake pid=%d\n",pid);
                    processPkt( ev );
                }
            }
        }

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

        void needRecv( StreamBase* stream ) {

    		m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "pid%d blocked, srcNode=%d srcPid=%d\n",
                    stream->getSrcPid(), stream->getSrcNode(), stream->getSrcPid() );

            m_nic.notifyNeedRecv( stream->getMyPid(), stream->getSrcNode(), stream->getSrcPid(), stream->length() );
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

        Nic&        m_nic;
        Output      m_dbg;
        int         m_vc;
        int         m_unit;
        int         m_rxMatchDelay;
        int         m_hostReadDelay;
        bool        m_notifyCallback; 
        FireflyNetworkEvent*        m_blockedNetworkEvent;

        std::vector< FireflyNetworkEvent* >             m_blockedNetworkEvents;
        std::vector< std::map< SrcKey, StreamBase*> >   m_streamMap;
        std::vector< std::map< int, DmaRecvEntry* > >   m_getOrgnM;
        std::vector< std::map< int, MemRgnEntry* > >    m_memRgnM;
        std::vector< std::deque<DmaRecvEntry*> >        m_postedRecvs;

#ifdef NIC_RECV_DEBUG 
        unsigned int    m_msgCount;
#endif
};

class CtlMsgRecvMachine : public RecvMachine {
  public:
    CtlMsgRecvMachine( Nic& nic, int vc, int numVnics, 
                int nodeId, int verboseLevel, int verboseMask,
                int rxMatchDelay, int hostReadDelay ) :
        RecvMachine( nic, vc, numVnics, nodeId, verboseLevel, verboseMask, rxMatchDelay, hostReadDelay )
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
        
        SendEntryBase* entry = findGet( ev->getDestPid(), ev->getSrcNode(), ev->getSrcPid(), rdmaHdr );
        assert(entry);

        ev->bufPop(sizeof(MsgHdr) + sizeof(rdmaHdr) );

        m_nic.schedCallback( 
                std::bind( &Nic::CtlMsgRecvMachine::queueResponse, this, entry ),
                m_hostReadDelay );

        delete ev;
    }
};
