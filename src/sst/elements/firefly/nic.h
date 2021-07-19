// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
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
#include <queue>
#include <sst/core/module.h>
#include <sst/core/component.h>
#include <sst/core/output.h>
#include <sst/core/link.h>

#include "sst/elements/hermes/shmemapi.h"
#include "sst/elements/thornhill/detailedCompute.h"
#include "ioVec.h"
#include "merlinEvent.h"
//#include "memoryModel/trivialMemoryModel.h"
#include "memoryModel/simpleMemoryModel.h"
#include "memoryModel/detailedInterface.h"

#define CALL_INFO_LAMBDA     __LINE__, __FILE__

namespace SST {
namespace Firefly {


#include "nicEvents.h"


#define NIC_DBG_DMA_ARBITRATE (1<<1)
#define NIC_DBG_DETAILED_MEM (1<<2)
#define NIC_DBG_SEND_MACHINE (1<<3)
#define NIC_DBG_RECV_MACHINE (1<<4)
#define NIC_DBG_SHMEM        (1<<5)
#define NIC_DBG_SEND_NETWORK (1<<6)
#define NIC_DBG_RECV_CTX     (1<<7)
#define NIC_DBG_RECV_STREAM  (1<<8)
#define NIC_DBG_RECV_MOVE    (1<<9)
#define NIC_DBG_LINK_CTRL    (1<<10)

#define STREAM_NUM_SIZE 12

class Nic : public SST::Component  {

  public:
    SST_ELI_REGISTER_COMPONENT(
        Nic,
        "firefly",
        "nic",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        COMPONENT_CATEGORY_SYSTEM
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "nid", "node id on network", "-1"},
        { "verboseLevel", "Sets the output verbosity of the component", "0"},
        { "verboseMask", "Sets the output mask of the component", "-1"},
        { "printConfig", "Controls whether configuration data is printed", "no"},
        { "nic2host_lat", "Sets the latency over the Host to NIC bus", "150ns"},
        { "numVNs","Number of VNs to be used","1"},
        { "getHdrVN", "VN to send headers on", "0"},
        { "getRespLargeVN", "VN to send large get responses on", "0"},
        { "getRespSmallVN", "VN to send small get responses on", "0"},
        { "getRespSize", "Currently Unused", "1500"},

        { "shmemAckVN", "VN to send Shmem acks on", "0"},
        { "shmemGetReqVN", "VN to send Shmem get requests on", "0"},
        { "shmemGetLargeVN", "VN to send large get responses on", "0"},
        { "shmemGetSmallVN", "VN to send small get responses on", "0"},
        { "shmemGetThresholdLength", "Currently unused", "0"},

        { "shmemPutLargeVN", "VN to send large puts on", "0"},
        { "shmemPutSmallVN", "VN to send small puts on", "0"},
        { "shmemPutThresholdLength", "Currently unused", "0"},

        { "rxMatchDelay_ns", "Sets the delay for a receive match", "100"},
        { "txDelay_ns", "Sets the delay for a send", "50"},
        { "hostReadDelay_ns", "Sets the delay for a read from the host", "200"},
        { "shmemRxDelay_ns", "Sets the delay for a SHMEM receive operation", "0"},
        { "num_vNics", "Sets number of cores", "1"},
        { "tracedPkt", "packet to trace", "-1"},
        { "tracedNode", "node to trace", "-1"},

        { "maxSendMachineQsize", "Sets the number of pending memory operations", "1"},
        { "maxRecvMachineQsize", "Sets the number of pending memory operations", "1"},
        { "shmemSendAlignment", "Sets the send stream transfer alignment", "64"},
        { "messageSendAlignment", "Sets the message alignment","1"},
        { "numSendMachines", "Sets the number of send machines", "1"},
        { "numRecvNicUnits", "Sets the number of receive units", "1"},
        { "nicAllocationPolicy", "Allocation policy for Nic", "RoundRobin"},
        { "packetOverhead", "Sets the overhead of a network packet", "0"},
        { "packetSize", "Sets the size of the network packet in bits or bytes" },

        { "corePortName", "Port connected to the core", "core"},

        { "shmem.nicCmdLatency", "Latency for posting shmem command on NIC", "10"},
        { "shmem.hostCmdLatency", "Host latency for posting shmem command", "10"},

        { "FAM_memsize", "", "0"},
        { "FAM_backed", "Controls whether FAM memory is backed in the simlation", "yes"},

        { "useSimpleMemoryModel", "If set to 1 use the simple memory model", "0"},
        { "useTrivialMemoryModel", "Use the trivial memory model", "false" },

        { "maxActiveRecvStreams", "Set max number of active receive streams", "16" },
        { "maxPendingRecvPkts", "Set max number of pending receive packets", "64" },

        { "dmaBW_GBs", "set the one way DMA bandwidth", "100"},
        { "dmaContentionMult", "set the DMA contention mult", "100"},

        {" useDetailed", "Use detailed compute model", "false"},
    )

	/* PARAMS
		shmem.*
		simpleMemoryModel.*
		detailedCompute.*
	*/

   SST_ELI_DOCUMENT_STATISTICS(
        { "sentByteCount",  "number of bytes sent on network", "bytes", 1},
        { "rcvdByteCount",  "number of bytes received from network", "bytes", 1},
        { "sentPkts",     	"number of packets sent on network", "packets", 1},
        { "rcvdPkts",       "number of packets received from network", "packets", 1},
        { "networkStall",   "number of picoseconds the outbound network port was blocked", "latency", 1},
        { "hostStall",      "number of nanoseconds the host blocked inbound network packets", "latency", 1},

        { "recvStreamPending",   "number of pending receive stream memory operations", "depth", 1},
        { "sendStreamPending",   "number of pending send stream memory operations", "depth", 1},

        { "detailed_num_reads",                "total number of loads", "count", 1},
        { "detailed_num_writes",               "total number of stores", "count", 1},
        { "detailed_req_latency",              "Running total of all latency for all requests", "count", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr", "Port connected to the router", {}},
        {"read", "Port connected to the detailed model", {}},
        {"write", "Port connected to the detailed model", {}},
        {"core%(num_vNics)d", "Ports connected to the network driver", {}},
        {"nicDetailedRead", "Port connected to the detailed model", {"memHierarchy.memEvent" , ""}},
        {"nicDetailedWrite", "Port connected to the detailed model", {"memHierarchy.memEvent" , ""}},
        {"detailed", "Port connected to the detailed model", {"memHierarchy.memEvent" , ""}},
    )

  private:
    typedef unsigned RespKey_t;
	class LinkControlWidget {

        typedef std::function<void()> Callback;
	  public:
		LinkControlWidget( Output& output, Callback callback, int numVN ) : m_dbg(output), m_notifiers(numVN,NULL), m_num(numVN,0), m_callback(callback) {
		}

		inline bool notify( int vn ) {
        	m_dbg.debug(CALL_INFO,1,NIC_DBG_LINK_CTRL,"Widget vn=%d\n",vn);
			if ( m_notifiers[vn] ) {
       			m_dbg.debug(CALL_INFO,1,NIC_DBG_LINK_CTRL,"Widget call notifier, number still installed %d\n", m_num[vn] - 1);
				m_notifiers[vn]();
				m_notifiers[vn] = NULL;
				--m_num[vn];
			}

			return m_num[vn] > 0;
		}

		inline void setNotify( std::function<void()> notifier, int vn ) {
        	m_dbg.debug(CALL_INFO,1, NIC_DBG_LINK_CTRL,"Widget vn=%d, number now installed %d\n",vn,m_num[vn] + 1);
			if ( m_num[vn] == 0 ) {
                m_callback();
			}
			assert( m_notifiers[vn] == NULL );
			m_notifiers[vn] = notifier;
			++m_num[vn];
		}

	  private:
        Callback m_callback;
        Output& m_dbg;
		std::vector< int > m_num;
		std::vector< std::function<void()> > m_notifiers;
	};

  public:

    typedef uint32_t NodeId;
    static const NodeId AnyId = -1;

	typedef MemoryModel::MemOp MemOp;

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
        unsigned short op : 3;
        unsigned short op2 : 3;
        unsigned short dataType : 3;
        uint32_t respKey : 24;

        std::string getOpStr( ) {
            switch(op){
            case Ack:
                return "Ack";
            case Put:
                return "Put";
            case Get:
                return "Get";
            case GetResp:
                return "GetResp";
            case Add:
                return "Add";
            case Fadd:
                return "Fadd";
            case Swap:
                return "Swap";
            case Cswap:
                return "Cswap";
            default:
                assert(0);
            }
        }
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

		enum { Callback, Event } type;
        typedef std::function<void()> Callback_t;

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

	class Priority {

 	  public:
    	Priority( SimTime_t p1, int p2 ) :  m_p1(p1), m_p2(p2) {}

    	SimTime_t p1() const { return m_p1; }
    	int p2() const { return m_p2; }
      private:
    	SimTime_t m_p1;
    	int m_p2;
	};

	class Compare {
  	  public:
    	bool operator()( Priority* lhs, Priority* rhs ) {
        	if ( lhs->p1() < rhs->p1() ) {
            	return false;
        	} else if ( lhs->p1() > rhs->p1() ) {
            	return true;
        	} else {
            	return lhs->p2() > rhs->p2();
        	}
    	}
	};

	template <class T>
	class PriorityEntry : public Priority {
	  public:
    	PriorityEntry( SimTime_t p1, int p2, T data ) : Priority( p1, p2), m_data(data) {}
    	T data() const { return m_data; }
  	  private:
    	T   m_data;
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
    #include "nicUnitPool.h"

    struct  RecvCtxData {
        std::unordered_map< int, DmaRecvEntry* >   m_getOrgnM;
        std::unordered_map< int, MemRgnEntry* >    m_memRgnM;
        std::deque<DmaRecvEntry*>        m_postedRecvs;
    };

public:

    typedef std::function<void()> Callback;
    Nic(ComponentId_t, Params& );
    ~Nic();

    void init( unsigned int phase );
    int getNodeId() { return m_myNodeId; }
    int getNum_vNics() { return m_num_vNics; }
    void printStatus(Output &out) {
        out.output("NIC %d: start time=%zu\n", m_myNodeId, (size_t) getCurrentSimTimeNano() );
        m_memoryModel->printStatus( out, m_myNodeId );
        out.output("NIC %d: done\n", m_myNodeId );
    }

	Statistic<uint64_t>* m_sentByteCount;
	Statistic<uint64_t>* m_rcvdByteCount;
	Statistic<uint64_t>* m_sentPkts;
	Statistic<uint64_t>* m_rcvdPkts;
	Statistic<uint64_t>* m_networkStall;
	Statistic<uint64_t>* m_hostStall;
	Statistic<uint64_t>* m_recvStreamPending;
	Statistic<uint64_t>* m_sendStreamPending;

    void detailedMemOp( Thornhill::DetailedCompute* detailed,
            std::vector<MemOp>& vec, std::string op, Callback callback );

    void dmaRead( int unit, int pid, std::vector<MemOp>* vec, Callback callback );
    void dmaWrite( int unit, int pid, std::vector<MemOp>* vec, Callback callback );

    void schedCallback( Callback callback, uint64_t delay = 0 ) {
        schedEvent( new SelfEvent( callback ), delay);
    }

    VirtNic* getVirtNic( int id ) {
        return m_vNicV[id];
    }

	void shmemDecPendingPuts( int core ) {
		m_shmem->decPendingPuts( core );
	}

    std::vector< RecvCtxData > m_recvCtxData;

  private:
    typedef uint64_t DestKey;
    static DestKey getDestKey(int node, int pid) { return (DestKey) node << 32 | pid; }

    std::queue<  std::pair< SimTime_t, SendEntryBase*> >   m_sendEntryQ;

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
		SimTime_t val = m_nic2host_lat_ns - ( m_nic2host_lat_ns > 0 ? 1 : 0 );
		return val; 
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
    	m_dbg.debug(CALL_INFO,2,1,"srcNode=%d srcPid=%d len=%lu\n", srcNode, srcPid, length);

        m_vNicV[pid]->notifyNeedRecv( srcPid, srcNode, length );
    }

    void notifyPutDone( int vNic, void* key ) {
        m_dbg.debug(CALL_INFO,2,1,"%p\n",key);
        assert(0);
    }

    void notifyGetDone( int vNic, int, int, int, size_t, void* key ) {
        m_dbg.debug(CALL_INFO,2,1,"%p\n",key);
        m_vNicV[vNic]->notifyGetDone( key );
    }

    uint16_t genGetKey() {
        return ++m_getKey;
    }

    bool sendNotify(int vn)
    {
        m_dbg.debug(CALL_INFO,2,1,"network can send on vn=%d\n",vn);
        return m_linkSendWidget->notify( vn );
    }


    bool recvNotify(int vn)
    {
        m_dbg.debug(CALL_INFO,2,1,"network event available vn=%d\n",vn);
        return m_linkRecvWidget->notify( vn );
    }

    int NetToId( int x ) { return x; }
    int IdToNet( int x ) { return x; }

struct X {
	X( Callback callback, FireflyNetworkEvent* pkt, int dest) : callback(callback), pkt(pkt), dest(dest) {}

	Callback			 callback;
	FireflyNetworkEvent* pkt;
	int                  dest;
};

	typedef PriorityEntry<X*> PriorityX;

	std::vector< std::priority_queue< PriorityX*,std::vector<PriorityX*>, Compare > > m_sendPQ;

    std::vector<SendMachine*>   m_sendMachineV;
    std::queue<SendMachine*>    m_sendMachineQ;
    std::vector<RecvMachine*>   m_recvMachine;
    ArbitrateDMA* m_arbitrateDMA;

    int                     m_myNodeId;
    int                     m_num_vNics;
    SST::Link*              m_selfLink;

    SST::Interfaces::SimpleNetwork*     m_linkControl;
    SST::Interfaces::SimpleNetwork::Handler<Nic>* m_recvNotifyFunctor;
    SST::Interfaces::SimpleNetwork::Handler<Nic>* m_sendNotifyFunctor;
    LinkControlWidget* m_linkRecvWidget;
    LinkControlWidget* m_linkSendWidget;

	uint64_t m_linkBytesPerSec;

    Output                  m_dbg;
    std::vector<VirtNic*>   m_vNicV;
    std::vector<Thornhill::DetailedCompute*> m_detailedCompute;
	DetailedInterface* m_detailedInterface;
	bool m_useDetailedCompute;
    Shmem* m_shmem;
	SimTime_t m_nic2host_lat_ns;
	SimTime_t m_shmemRxDelay_ns;

    UnitPool* m_unitPool;

    void feedTheNetwork( int vn );
    void sendPkt( FireflyNetworkEvent*, int dest, int vn );
    void notifySendDone( SendMachine* mach, SendEntryBase* entry );

    void qSendEntry( SendEntryBase* entry );

    void notifyHavePkt( PriorityX* px, int vn ) {
        m_dbg.debug(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"vn=%d p1=%" PRIu64 " p2=%d\n",vn,px->p1(),px->p2());
        m_sendPQ[vn].push( px );

        if ( 1 == m_sendPQ[vn].size() ) {
            feedTheNetwork( vn );
        }
    }

    void calcHostMemDelay( int core, std::vector< MemOp>* ops, std::function<void()> callback  ) {
        if( m_memoryModel ) {
        	m_memoryModel->schedHostCallback( core, ops, callback );
        } else {
			schedCallback(callback);
			delete ops;
		}
    }

    int allocNicAckUnit() {
        return m_unitPool->allocAckUnit();
    }

    int allocNicSendUnit() {
        return m_unitPool->allocSendUnit();
    }

    int allocNicRecvUnit( int pid ) {
        return m_unitPool->allocRecvUnit( pid );
    }

    void calcNicMemDelay( int unit, int pid, std::vector< MemOp>* ops, std::function<void()> callback ) {
        if( m_memoryModel ) {
        	m_memoryModel->schedNicCallback( unit, pid, ops, callback );
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
        m_dbg.debug(CALL_INFO,2,1,"map[%#x]=%p nextkey=%#x\n",key-1,ptr,m_respKey);
        return key;
    }
    void* getRespKeyValue( RespKey_t key ) {
        void* value = m_respKeyMap[key];
        m_dbg.debug(CALL_INFO,2,1,"map[%#x]=%p\n",key,value);
        m_respKeyMap.erase(key);
        return value;
    }
	bool findNid( int nid, std::string nidList ) {

		//printf("%s() %d %s\n",__func__,nid,nidList.c_str());

		if ( 0 == nidList.compare( "all" ) ) {
			return true;
		}

		if ( nidList.empty() ) {
			return false;
		}

		size_t pos = 0;
		size_t end = 0;
		do {
			end = nidList.find( ",", pos );
			if ( end == std::string::npos ) {
				end = nidList.length();
			}
			std::string tmp = nidList.substr(pos,end-pos);
			//printf("pos=%d end=%d '%s'\n",pos,end,tmp.c_str() );

			if ( tmp.length() == 1 ) {
				int val = atoi( tmp.c_str() );
				//printf("nid=%d val=%d\n",nid, val);
				if ( nid == val ) {
					return true;
				}
			} else {
				size_t dash = tmp.find( "-" );
				int first = atoi(tmp.substr(0,dash).c_str()) ;
				int last = atoi(tmp.substr(dash+1).c_str());
				//printf("nid=%d first=%d last=%d\n",nid, first,last);
				if ( nid >= first && nid <= last ) {
					return true;
				}
			}

			pos = end + 1;
		} while ( end < nidList.length() );

		return false;
	}

    std::unordered_map<RespKey_t,void*> m_respKeyMap;

    MemoryModel*  m_memoryModel;
    std::queue<int> m_availNicUnits;
    uint16_t m_getKey;
    int m_txDelay;

    int m_numVN;

    static int  MaxPayload;
    static int  m_packetId;
	int m_tracedPkt;
	int m_tracedNode;
	SimTime_t m_predNetIdleTime;

    int m_getHdrVN;
    int m_getRespSize;
    int m_getRespLargeVN;
    int m_getRespSmallVN;


    int m_shmemAckVN;

    int m_shmemGetReqVN;
    int m_shmemGetLargeVN;
    int m_shmemGetSmallVN;
    size_t m_shmemGetThresholdLength;

    int m_shmemPutLargeVN;
    int m_shmemPutSmallVN;
    size_t m_shmemPutThresholdLength;
};

} // namesapce Firefly
} // namespace SST

#endif
