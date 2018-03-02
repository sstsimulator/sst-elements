// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_FIREFLY_NIC_H 
#define COMPONENTS_FIREFLY_NIC_H 

#include <math.h>
#include <sstream>
#include <sst/core/module.h>
#include <sst/core/component.h>
#include <sst/core/output.h>
#include <sst/core/link.h>

#include "sst/elements/hermes/shmemapi.h"
#include "sst/elements/thornhill/detailedCompute.h"
#include "ioVec.h"
#include "merlinEvent.h"
#include "memoryModel/simpleMemoryModel.h"

namespace SST {
namespace Firefly {

#include "nicEvents.h"

#define NIC_DBG_DMA_ARBITRATE 1<<1
#define NIC_DBG_DETAILED_MEM 1<<2
#define NIC_DBG_SEND_MACHINE 1<<3
#define NIC_DBG_RECV_MACHINE 1<<4
#define NIC_SHMEM 1 << 5 

class Nic : public SST::Component  {

    typedef unsigned RespKey_t;
	class LinkControlWidget {

	  public:
		LinkControlWidget( Nic* nic) : m_nic(nic), m_notifiers(2,NULL), m_num(0) {
		}

		inline bool notify( int vn ) {
        	m_nic->m_dbg.verbose(CALL_INFO,2,1,"Widget vn=%d\n",vn);
			if ( m_notifiers[vn] ) {
       			m_nic->m_dbg.verbose(CALL_INFO,2,1,"Widget call notifier num=%d\n", m_num -1);
				m_notifiers[vn]();
				m_notifiers[vn] = NULL;
				--m_num;
			}

			return m_num > 0; 
		}

		inline void setNotifyOnReceive( std::function<void()> notifier, int vn ) {
        	m_nic->m_dbg.verbose(CALL_INFO,2,1,"Widget vn=%d num=%d\n",vn,m_num);
			if ( m_num == 0 ) {
				m_nic->setRecvNotifier();
			}
			assert( m_notifiers[vn] == NULL );
			m_notifiers[vn] = notifier;
			++m_num;
		}

	  private:
		Nic* m_nic;
		int m_num;
		std::vector< std::function<void()> > m_notifiers;
	};

  public:

    typedef uint32_t NodeId;
    static const NodeId AnyId = -1;

	typedef SimpleMemoryModel::MemOp MemOp;

  private:

    struct __attribute__ ((packed)) MsgHdr {
        enum Op : unsigned char { Msg, Rdma, Shmem } op;
    };

    struct __attribute__ ((packed)) MatchMsgHdr {
        size_t len;
        int    tag;
    };

    struct __attribute__ ((packed)) ShmemMsgHdr {
        ShmemMsgHdr() : op2(0) {}
        uint64_t vaddr;
        uint32_t length;
        enum Op { Ack, Put, Get, GetResp, Add, Fadd, Swap, Cswap };
        unsigned char op : 3; 
        unsigned char op2 : 3; 
        unsigned char dataType : 3;
        uint32_t respKey : 24;
        uint8_t pad[46]; // we want the payload of a stream to be aligned on cache line,
                         // so packetSize - stream headers (MsgHdr and this header) should be a multiple of a cache line
    };
    struct RdmaMsgHdr {
        enum { Put, Get, GetResp } op;
        uint16_t    rgnNum;
        uint16_t    respKey;
        uint32_t    offset;
    };

    class EntryBase;
    class SelfEvent : public SST::Event {
      public:

		enum { Callback, Entry, Event } type;
        typedef std::function<void()> Callback_t;

        SelfEvent( void* entry) : 
            type(Entry), entry(entry) {}
        SelfEvent( Callback_t callback ) :
            type(Callback), callback( callback) {}
        SelfEvent( SST::Event* ev,  int linkNum  ) :
            type(Event), event( ev), linkNum(linkNum) {}
        
        void*              entry;
        Callback_t         callback;
		SST::Event*        event;
		int				   linkNum;
        
        NotSerializable(SelfEvent)
    };

    class MemRgnEntry {
      public:
        MemRgnEntry( std::vector<IoVec>& iovec ) : 
            m_iovec( iovec )
        { }

        std::vector<IoVec>& iovec() { return m_iovec; } 

      private:
        std::vector<IoVec>  m_iovec;
        
    };

    #include "nicVirtNic.h" 
    #include "nicShmem.h"
    #include "nicShmemMove.h" 
    #include "nicEntryBase.h"
    #include "nicSendEntry.h"
    #include "nicShmemSendEntry.h" 
    #include "nicRecvEntry.h"
    #include "nicSendMachine.h"
    #include "nicRecvMachine.h"
    #include "nicArbitrateDMA.h"

public:

    typedef std::function<void()> Callback;
    Nic(ComponentId_t, Params& );
    ~Nic();

    void init( unsigned int phase );
    int getNodeId() { return m_myNodeId; }
    int getNum_vNics() { return m_num_vNics; }
    void printStatus(Output &out);

    void detailedMemOp( Thornhill::DetailedCompute* detailed,
            std::vector<MemOp>& vec, std::string op, Callback callback );

    void dmaRead( int unit, int pid, std::vector<MemOp>* vec, Callback callback );
    void dmaWrite( int unit, int pid, std::vector<MemOp>* vec, Callback callback );

    void schedCallback( Callback callback, uint64_t delay = 0 ) {
        schedEvent( new SelfEvent( callback ), delay);
    }

    void setNotifyOnSend( int vc ) {
        assert( ! m_sendNotify[vc] ); 
        m_sendNotify[vc] = true;
        ++m_sendNotifyCnt;
        m_linkControl->setNotifyOnSend( m_sendNotifyFunctor );
    }

    std::vector<bool> m_sendNotify;
    int m_sendNotifyCnt;
    VirtNic* getVirtNic( int id ) {
        return m_vNicV[id];
    }
	void shmemDecPending( int core ) {
		m_shmem->decPending( core );
	}

  private:
    void handleSelfEvent( Event* );
    void handleVnicEvent( Event*, int );
    void handleVnicEvent2( Event*, int );
    void handleMsgEvent( NicCmdEvent* event, int id );
    void handleShmemEvent( NicShmemCmdEvent* event, int id );
    void dmaSend( NicCmdEvent*, int );
    void pioSend( NicCmdEvent*, int );
    void dmaRecv( NicCmdEvent*, int );
    void get( NicCmdEvent*, int );
    void put( NicCmdEvent*, int );
    void regMemRgn( NicCmdEvent*, int );
    void processNetworkEvent( FireflyNetworkEvent* );

    Hermes::MemAddr findShmem( int core, Hermes::Vaddr  addr, size_t length );

	SimTime_t getShmemRxDelay_ns() {
		return m_shmemRxDelay_ns; 
	}

	SimTime_t calcDelay_ns( SST::UnitAlgebra val ) {
		if ( val.hasUnits("ns") ) {
			return val.getValue().toDouble()* 1000000000;
		}
		assert(0);
	}
	SimTime_t getDelay_ns( ) {
		return m_nic2host_lat_ns - m_nic2host_base_lat_ns;
	}

    void schedEvent( SelfEvent* event, SimTime_t delay = 0 ) {
        m_selfLink->send( delay, event );
    }

    void notifySendDmaDone( int vNicNum, void* key ) {
        m_vNicV[vNicNum]->notifySendDmaDone(  key );
    }

    void notifySendPioDone( int vNicNum, void* key ) {
        m_vNicV[vNicNum]->notifySendPioDone(  key );
    }

    void notifyRecvDmaDone(int vNic, int src_vNic, int src, int tag,
                                                    size_t len, void* key) {
        m_vNicV[vNic]->notifyRecvDmaDone( src_vNic, src, tag, len, key );
    }

    void notifyNeedRecv( int pid, int srcNode, int srcPid, size_t length ) {
    	m_dbg.verbose(CALL_INFO,2,1,"srcNode=%d srcPid=%d len=%lu\n", srcNode, srcPid, length);

        m_vNicV[pid]->notifyNeedRecv( srcPid, srcNode, length );
    }

    void notifyPutDone( int vNic, void* key ) {
        m_dbg.verbose(CALL_INFO,2,1,"%p\n",key);
        assert(0);
    }

    void notifyGetDone( int vNic, int, int, int, size_t, void* key ) {
        m_dbg.verbose(CALL_INFO,2,1,"%p\n",key);
        m_vNicV[vNic]->notifyGetDone( key );
    }

    uint16_t genGetKey() {
        return ++m_getKey;
    }

    void setRecvNotifier() {
        m_dbg.verbose(CALL_INFO,2,1,"\n");
        m_linkControl->setNotifyOnReceive( m_recvNotifyFunctor );
    }

    int NetToId( int x ) { return x; }
    int IdToNet( int x ) { return x; }

    std::vector<SendMachine*> m_sendMachine;
    std::vector<RecvMachine*> m_recvMachine;
    ArbitrateDMA* m_arbitrateDMA;

    int                     m_myNodeId;
    int                     m_num_vNics;
    SST::Link*              m_selfLink;

    // the interface to to Merlin
    // Merlin::LinkControl*                m_linkControl;
    // Merlin::LinkControl::Handler<Nic> * m_recvNotifyFunctor;
    // Merlin::LinkControl::Handler<Nic> * m_sendNotifyFunctor;
    SST::Interfaces::SimpleNetwork*     m_linkControl;
    SST::Interfaces::SimpleNetwork::Handler<Nic>* m_recvNotifyFunctor;
    SST::Interfaces::SimpleNetwork::Handler<Nic>* m_sendNotifyFunctor;
    bool sendNotify(int);
    bool recvNotify(int);
    LinkControlWidget m_linkWidget;

    Output                  m_dbg;
    std::vector<VirtNic*>   m_vNicV;
    std::vector<Thornhill::DetailedCompute*> m_detailedCompute;
	bool m_useDetailedCompute;
    Shmem* m_shmem;
	SimTime_t m_nic2host_lat_ns;
	SimTime_t m_nic2host_base_lat_ns;
	SimTime_t m_shmemRxDelay_ns; 
	int m_numNicUnits;
    int m_curNicUnit;
    int m_numNicUnitsPerCtx;
    enum { RoundRobin, PerContext } m_nicUnitAllocPolicy;

    void calcHostMemDelay( int core, std::vector< MemOp>* ops, std::function<void()> callback  ) {
        if( m_simpleMemoryModel ) {
        	m_simpleMemoryModel->schedHostCallback( core, ops, callback );
        } else {
			schedCallback(callback);
			delete ops;
		}
    }

    void initNicUnitPool( int numUnits, int numCtx, std::string policy ) {
        m_numNicUnits = numUnits;
        m_dbg.verbose(CALL_INFO,3,1,"num=%d policy=%s\n",numUnits, policy.c_str());
        if ( 0 == policy.compare("RoundRobin") ) {
            m_nicUnitAllocPolicy = RoundRobin;
            assert(0);
        } else if ( 0 == policy.compare("PerContext") ) {
            m_nicUnitAllocPolicy = PerContext;

            m_numNicUnitsPerCtx = m_numNicUnits/numCtx;
        } else {
            assert(0);
        }
    }

    int allocNicUnit( int pid, int num ) {
        int unit;
        switch ( m_nicUnitAllocPolicy ) {
          case RoundRobin:
            unit = m_curNicUnit++ % m_numNicUnits;            
            break;
          case PerContext:
            assert( num < m_numNicUnitsPerCtx ); 
            unit = pid * m_numNicUnitsPerCtx + num;
            break;
          default:
            assert(0);
        }

        m_dbg.verbose(CALL_INFO,3,1,"pid=%d unit=%d\n",pid, unit);
        return unit;
    }

    void freeNicUnit( int unit ) {
        m_dbg.verbose(CALL_INFO,3,1,"unit=%d\n",unit);
    }

    void calcNicMemDelay( int unit, int pid, std::vector< MemOp>* ops, std::function<void()> callback ) {
        if( m_simpleMemoryModel ) {
        	m_simpleMemoryModel->schedNicCallback( unit, pid, ops, callback );
        } else {
        	for ( unsigned i = 0;  i <  ops->size(); i++ ) {
            	assert( (*ops)[i].callback == NULL );
        	}
			schedCallback(callback);
			delete ops;
		}
	}

    RespKey_t m_respKey;
    RespKey_t genRespKey( void* ptr ) {
        assert( m_respKeyMap.find(m_respKey) == m_respKeyMap.end() );
        RespKey_t key = m_respKey++;
        m_respKeyMap[key] = ptr;  
        m_respKey &= 0xffffff;
        if ( 0 == m_respKey ) {
            ++m_respKey;
        }
        m_dbg.verbose(CALL_INFO,2,1,"map[%#x]=%p nextkey=%#x\n",key-1,ptr,m_respKey);
        return key;
    }
    void* getRespKeyValue( RespKey_t key ) {
        void* value = m_respKeyMap[key];
        m_dbg.verbose(CALL_INFO,2,1,"map[%#x]=%p\n",key,value);
        m_respKeyMap.erase(key);
        return value; 
    }

    std::map<RespKey_t,void*> m_respKeyMap;

    SimpleMemoryModel*  m_simpleMemoryModel;
    std::deque<int> m_availNicUnits;
    uint16_t m_getKey;
  public:
	int m_tracedPkt;
	int m_tracedNode;
}; 

} // namesapce Firefly 
} // namespace SST

#endif
