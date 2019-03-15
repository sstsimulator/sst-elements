// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_CTRLMSGPROCESSQUEUESSTATE_H
#define COMPONENTS_FIREFLY_CTRLMSGPROCESSQUEUESSTATE_H

#include <functional>
#include <stdint.h>
#include "ctrlMsg.h"
#include <sst/core/elementinfo.h>
#include <sst/core/output.h>

#include "info.h"
#include "heapAddrs.h"
#include "ctrlMsgTiming.h"
#include "loopBack.h"

#include "ctrlMsgCommReq.h"
#include "ctrlMsgWaitReq.h"

#define DBG_MSK_PQS_APP_SIDE 1 << 0
#define DBG_MSK_PQS_INT 1 << 1 
#define DBG_MSK_PQS_Q 1 << 2 
#define DBG_MSK_PQS_CB 1 << 3 
#define DBG_MSK_PQS_LOOP 1<< 4
#define DBG_MSK_PQS_NEED_RECV 1<< 5 
#define DBG_MSK_PQS_POST_SHORT 1<< 6 


namespace SST {
namespace Firefly {
namespace CtrlMsg {

static const int    ShortMsgQ       = 0xf00d;

static const key_t LongGetKey  = 1 << (sizeof(key_t) * 8 - 2);
static const key_t LongAckKey = LongGetKey;
static const key_t LongRspKey  = 2 << (sizeof(key_t) * 8 - 2);
static const key_t ReadReqKey  = 3 << (sizeof(key_t) * 8 - 2);
static const key_t ReadRspKey  = 0 << (sizeof(key_t) * 8 - 2);

typedef int region_t;

class MemoryBase;

class ProcessQueuesState : public SubComponent
{
  public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        ProcessQueuesState,
        "firefly",
        "ctrlMsg",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"shortMsgLength","Sets the short to long message transition point", "16000"},
        {"verboseLevel","Set the verbose level", "1"},
        {"debug","Set the debug level", "0"},
        {"txMemcpyMod","Set the module used to calculate TX mempcy latency", ""},
        {"rxMemcpyMod","Set the module used to calculate RX mempcy latency", ""},
        {"matchDelay_ns","Sets the time to do a match", "100"},
        {"txSetupMod","Set the module used to calculate TX setup latency", ""},
        {"rxSetupMod","Set the module used to calculate RX setup latency", ""},
        {"txFiniMod","Set the module used to calculate TX fini latency", ""},
        {"rxFiniMod","Set the module used to calculate RX fini latency", ""},
        {"rxPostMod","Set the module used to calculate RX post latency", ""},
        {"loopBackPortName","Sets port name to use when connecting to the loopBack component","loop"},
        {"rxNicDelay_ns","", "0"},
        {"txNicDelay_ns","", "0"},
        {"sendReqFiniDelay_ns","", "0"},
        {"recvReqFiniDelay_ns","", "0"},
        {"sendAckDelay_ns","", "0"},
        {"regRegionXoverLength","Sets the transition point page pinning", "4096"},
        {"regRegionPerPageDelay_ns","Sets the time to pin pages", "0"},
        {"regRegionBaseDelay_ns","Sets the base time to pin pages", "0"},
        {"sendStateDelay_ps","", "0"},
        {"recvStateDelay_ps","", "0"},
        {"waitallStateDelay_ps","", "0"},
        {"waitanyStateDelay_ps","", "0"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "posted_receive_list", "", "count", 1 },
        { "received_msg_list", "", "count", 1 }
    )

  private:
    typedef std::function<void(nid_t, uint32_t, size_t)> Callback2;
    typedef std::function<void()> VoidFunction;

    class DelayEvent : public SST::Event {
      public:

        DelayEvent( VoidFunction _callback ) :
            Event(),
            callback( _callback )
        {}

        VoidFunction                callback;

        NotSerializable(DelayEvent)
    };

    class LoopBackEvent : public LoopBackEventBase {
      public:
        LoopBackEvent( std::vector<IoVec>& _vec, int core, void* _key ) :
            LoopBackEventBase( core ), vec( _vec ), key( _key ), response( false )
        {}

        LoopBackEvent( int core, void* _key ) : LoopBackEventBase( core ), key( _key ), response( true )
        {}

        std::vector<IoVec>  vec;
        void*               key;
        bool                response;

        NotSerializable(LoopBackEvent)
    };

    static const unsigned long MaxPostedShortBuffers = 512;
    static const unsigned long MinPostedShortBuffers = 5;


  public:
    ProcessQueuesState( Component* owner, Params& params );
    ~ProcessQueuesState();
    void setup() {}
    void finish();

    void setVars( VirtNic* nic, Info* info, MemoryBase* mem, 
        Thornhill::MemoryHeapLink* m_memHeapLink, Link* returnToCaller );

    void enterInit( bool );
    void enterSend( _CommReq*, uint64_t exitDelay = 0 );
    void enterRecv( _CommReq*, uint64_t exitDelay = 0 );
    void enterWait( WaitReq*, uint64_t exitDelay = 0 );
    void enterMakeProgress( uint64_t exitDelay = 0 );

    void needRecv( int, size_t );

  private:

    void loopHandler( Event* );
    void delayHandler( Event* );

    struct CtrlHdr {
        key_t     key;
    };

    class Msg {
      public:
        Msg( MatchHdr* hdr ) : m_hdr( hdr ) {}
        virtual ~Msg() {}
        MatchHdr& hdr() { return *m_hdr; }
        std::vector<IoVec>& ioVec() { return m_ioVec; }

      protected:
        std::vector<IoVec> m_ioVec;

      private:
        MatchHdr* m_hdr;
    };

    class ShortRecvBuffer : public Msg {
      public:
        ShortRecvBuffer(size_t length, HeapAddrs& _heap  ) : Msg( &hdr ), heap(_heap) 
        { 
			if ( length ) {
            	ioVec.resize(2);    
			} else {
            	ioVec.resize(1);    
			}

            ioVec[0].len = sizeof(hdr);
            ioVec[0].addr.setSimVAddr( heap.alloc(ioVec[0].len) );
            ioVec[0].addr.setBacking( &hdr );

            if ( length ) {
                buf.resize( length );
            	ioVec[1].len = length;
            	ioVec[1].addr.setSimVAddr( heap.alloc(length) );
            	ioVec[1].addr.setBacking( &buf[1] );
            } else {
				assert(0);
			}

            m_ioVec.push_back( ioVec[1] ); 
        }
		~ShortRecvBuffer() {
			for ( unsigned i = 0; i < ioVec.size(); i++) {
				heap.free( ioVec[i].addr.getSimVAddr() );
			}
		}

        MatchHdr                hdr;
        std::vector<IoVec>      ioVec; 
        std::vector<unsigned char> buf;
        HeapAddrs& 				heap;
    };

    struct LoopReq : public Msg {
        LoopReq(int _srcCore, std::vector<IoVec>& _vec, void* _key ) :
            Msg( (MatchHdr*)_vec[0].addr.getBacking() ), 
            srcCore( _srcCore ), vec(_vec), key( _key) 
        {
            m_ioVec.push_back( vec[1] ); 
        }

        int srcCore;
        std::vector<IoVec>& vec;
        void* key;
    };

    struct LoopResp{
        LoopResp( int _srcCore, void* _key ) : srcCore(_srcCore), key(_key) {}
        int srcCore;
        void* key;
    };

    struct GetInfo {
        _CommReq*  req;
        CtrlHdr    hdr;
        nid_t      nid;
        uint32_t   tag;
        size_t     length;
    };

    class FuncCtxBase {
      public:
        FuncCtxBase( VoidFunction ret = NULL ) : 
			callback(ret) {}

		virtual ~FuncCtxBase() {}
        VoidFunction getCallback() {
			return callback;
		} 

		void setCallback( VoidFunction arg ) {
			callback = arg;
		}

	  private:
        VoidFunction callback;
    };

    typedef std::deque<FuncCtxBase*> Stack; 

    class ProcessQueuesCtx : public FuncCtxBase {
	  public:
		ProcessQueuesCtx( VoidFunction callback ) :
			FuncCtxBase( callback ) {}
    };
	
    class InterruptCtx : public FuncCtxBase {
	  public:
		InterruptCtx( VoidFunction callback ) :
			FuncCtxBase( callback ) {}
    };

    class ProcessShortListCtx : public FuncCtxBase {
      public:
        ProcessShortListCtx( std::deque<Msg*>& q ) : 
            m_msgQ(q), m_iter(m_msgQ.begin()) { }

        MatchHdr&   hdr() {
            return (*m_iter)->hdr();
        }

        std::vector<IoVec>& ioVec() { 
     	 	return (*m_iter)->ioVec();
        }

        Msg* msg() {
            return *m_iter;
        }
        std::deque<Msg*>& getMsgQ() { return m_msgQ; }
        bool msgQempty() { return m_msgQ.empty(); } 

        _CommReq*    req; 

        void removeMsg() { 
            delete *m_iter;
            m_iter = m_msgQ.erase(m_iter);
        }

        bool isDone() { return m_iter == m_msgQ.end(); }
        void incPos() { ++m_iter; }
      private:
        std::deque<Msg*> 			m_msgQ; 
        typename std::deque<Msg*>::iterator 	m_iter;
    };

    class WaitCtx : public FuncCtxBase {
      public:
		WaitCtx( WaitReq* _req, VoidFunction callback ) :
			FuncCtxBase( callback ), req( _req ) {}

        ~WaitCtx() { delete req; }

        WaitReq*    req;
    };

    class ProcessLongGetFiniCtx : public FuncCtxBase {
      public:
		ProcessLongGetFiniCtx( _CommReq* _req ) : req( _req ) {}

        ~ProcessLongGetFiniCtx() { }

        _CommReq*    req;
    };

    void enterInit_1( uint64_t addr, size_t length );

    void processLoopResp( LoopResp* );
    void processLongAck( GetInfo* );
    void processLongGetFini( Stack*, _CommReq* );
    void processLongGetFini0( Stack* );
    void processPioSendFini( _CommReq* );

    void processSend_0( _CommReq* );
    void processSend_1( _CommReq* );
    void processSend_2( _CommReq* );
    void processSendLoop( _CommReq* );

    void processRecv_0( _CommReq* );
    void processRecv_1( _CommReq* );

    void processMakeProgress( Stack* );

    void processWait_0( Stack* );
    void processWaitCtx_0( WaitCtx*, _CommReq* req );
    void processWaitCtx_1( WaitCtx*, _CommReq* req );
    void processWaitCtx_2( WaitCtx* );

    void processQueues( Stack* );
    void processQueues0( Stack* );

    void processShortList_0( Stack* );
    void processShortList_1( Stack* );
    void processShortList_2( Stack* );
    void processShortList_3( Stack* );
    void processShortList_4( Stack* );
    void processShortList_5( Stack* );

	void enableInt( FuncCtxBase*, void (ProcessQueuesState::*)( Stack*) );

    void runInterruptCtx();
    void leaveInterruptCtx( Stack* );

    void copyIoVec( std::vector<IoVec>& dst, std::vector<IoVec>& src, size_t);

    Output& dbg()   { return m_dbg; }

    void postShortRecvBuffer();

    void pioSendFiniVoid( void*, uint64_t );
    void pioSendFiniCtrlHdr( CtrlHdr*, uint64_t );
    void getFini( _CommReq* );
    void dmaRecvFiniGI( GetInfo*, uint64_t, nid_t, uint32_t, size_t );
    void dmaRecvFiniSRB( ShortRecvBuffer*, nid_t, uint32_t, size_t );


    bool        checkMatchHdr( MatchHdr& hdr, MatchHdr& wantHdr, uint64_t ignore );
    _CommReq*	searchPostedRecv( MatchHdr& hdr, int& delay );

    void exit( int delay = 0 ) {
        dbg().debug(CALL_INFO,2,DBG_MSK_PQS_APP_SIDE,"exit ProcessQueuesState\n"); 
        passCtrlToFunction( m_exitDelay + delay );
        m_exitDelay = 0;
    }

    key_t    genGetKey() {
        key_t tmp = m_getKey;;
        ++m_getKey;
        if ( m_getKey == LongGetKey ) m_getKey = 0; 
        return tmp | LongGetKey;
    }  

    key_t    genRspKey() {
        key_t tmp = m_rspKey;
        ++m_rspKey;
        if ( m_rspKey == LongRspKey ) m_rspKey = 0; 
        return tmp | LongRspKey;
    }  

    key_t    genReadReqKey( region_t region ) {
        //assert ( region to big ); 
        return region | ReadReqKey;
    }  

    key_t    genReadRspKey( region_t region ) {
        //assert ( region to big ); 
        return region | ReadRspKey;
    }  

    nid_t calcNid( _CommReq* req, MP::RankID rank ) {
        return m_info->getGroup( req->getGroup() )->getMapping( rank );
    }

    MP::RankID getMyRank( _CommReq* req ) {
        return m_info->getGroup( req->getGroup() )->getMyRank();
    } 

    size_t shortMsgLength( ) {
        return m_msgTiming->shortMsgLength();
    }
    uint64_t txDelay( size_t bytes ) {
        return m_msgTiming->txDelay( bytes );
    }
    uint64_t rxDelay( size_t bytes ) {
        return m_msgTiming->rxDelay( bytes );
    }
    uint64_t sendReqFiniDelay( size_t bytes ) {
        return m_msgTiming->sendReqFiniDelay( bytes );
    }
    uint64_t recvReqFiniDelay( size_t bytes ) {
        return m_msgTiming->recvReqFiniDelay( bytes );
    }
    uint64_t rxPostDelay_ns( size_t bytes ) {
        return m_msgTiming->rxPostDelay_ns( bytes );
    }
    virtual uint64_t sendAckDelay() {
        return m_msgTiming->sendAckDelay();
    }

    void schedCallback( VoidFunction callback, uint64_t delay = 0) {
        m_delayLink->send( delay, new DelayEvent(callback) );
    }  
    void passCtrlToFunction( uint64_t delay = 0 ) {
        dbg().debug(CALL_INFO,1,DBG_MSK_PQS_APP_SIDE,"\n"); 
        m_returnToCaller->send( delay, NULL );
    }

    void memHeapAlloc( size_t bytes, std::function<void(uint64_t)> callback ) {
        m_memHeapLink->alloc( bytes, callback );
    }

    void loopHandler( int, std::vector<IoVec>&, void* );
    void loopHandler( int, void* );
    void loopSendReq( std::vector<IoVec>&, int, void* );
    void loopSendResp( int, void* );

    Output      m_dbg;
    VirtNic*    m_nic;
    Info*       m_info;
    MemoryBase* m_mem;
    Thornhill::MemoryHeapLink* m_memHeapLink;
    MsgTiming*  m_msgTiming;


    key_t   m_getKey;
    key_t   m_rspKey;
    int     m_needRecv;
    int     m_numNicRequestedShortBuff;
    int     m_numRecvLooped;
    bool    m_missedInt;

    std::deque< _CommReq* >         m_pstdRcvQ;
    std::deque< Msg* >              m_recvdMsgQ;

    std::deque< _CommReq* >         m_longGetFiniQ;
    std::deque< GetInfo* >          m_longAckQ;

    Stack                           m_funcStack; 
    Stack                           m_intStack; 

    std::deque< LoopResp* >         m_loopResp;

    std::map< ShortRecvBuffer*, Callback2* > m_postedShortBuffers; 
	FuncCtxBase*	m_intCtx;
    uint64_t        m_exitDelay;

    HeapAddrs*      m_simVAddrs;

    Link*   m_loopLink;
    Link*   m_delayLink;
    Link*   m_returnToCaller;

    Statistic<uint64_t>* m_statRcvdMsg;
    Statistic<uint64_t>* m_statPstdRcv;
    int m_numSent;
    int m_numRecv;
};

}
}
}

#endif
