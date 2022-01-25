// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_SHMEM_NIC_H
#define MEMHIERARCHY_SHMEM_NIC_H

#include <sst/core/sst_types.h>

#include <sst/core/component.h>
#include <sst/core/event.h>

#include <sst/core/interfaces/stdMem.h>

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/memLinkBase.h"

#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "rdmaNicHostInterface.h"

#define DBG_X_FLAG (1<<0)
#define DBG_MEMEVENT_FLAG (1<<1)
#define DBG_FAM_FLAG (1<<2)

using namespace SST::Interfaces;

namespace SST {
namespace MemHierarchy {

#include "rdmaNicNetworkEvent.h"

class RdmaNic : public SST::Component {
  public:

    SST_ELI_REGISTER_COMPONENT(RdmaNic, "memHierarchy", "RdmaNic", SST_ELI_ELEMENT_VERSION(1,0,0),
        "RDMA NIC, interfaces to a main memory model as memory and CPU", COMPONENT_CATEGORY_MEMORY)

#define MEMCONTROLLER_ELI_PORTS \
            {"direct_link", "Direct connection to a cache/directory controller", {"memHierarchy.MemEventBase"} },\
            {"network",     "Network connection to a cache/directory controller; also request network for split networks", {"memHierarchy.MemRtrEvent"} },\
            {"network_ack", "For split networks, ack/response network connection to a cache/directory controller", {"memHierarchy.MemRtrEvent"} },\
            {"network_fwd", "For split networks, forward request network connection to a cache/directory controller", {"memHierarchy.MemRtrEvent"} },\
            {"network_data","For split networks, data network connection to a cache/directory controller", {"memHierarchy.MemRtrEvent"} },\
            {"cache_link",  "Link to Memory Controller", { "memHierarchy.memEvent" , "" } }, \

    SST_ELI_DOCUMENT_STATISTICS(
        { "cyclePerIncOpRead",    "latency of inc op",     "ns", 1 },
        { "cyclePerIncOp",    "latency of inc op",     "ns", 1 },
        { "addrIncOp",    "addr of memory access",     "addr", 1 },
        { "pendingMemResp",  "number of mem request waiting for response",   "request", 1 },
        { "pendingCmds",  "number of SHMEM commands current in process",   "request", 1 },
        { "localQ_0",  "",   "request", 1 },
        { "localQ_1",  "",   "request", 1 },
        { "remoteQ_0",  "",   "request", 1 },
        { "remoteQ_1",  "",   "request", 1 },
        { "hostCmdQ",  "",   "request", 1 },
        { "headUpdateQ",  "",   "request", 1 },
        { "FamGetLatency",  "",   "request", 1 },
        { "hostToNicLatency",  "",   "request", 1 }
    )

    SST_ELI_DOCUMENT_PORTS( MEMCONTROLLER_ELI_PORTS )

    RdmaNic(ComponentId_t id, Params &params);

    virtual void init(unsigned int);
    virtual void setup();
    void finish();

	typedef RdmaNicNetworkEvent::StreamId StreamId;
  protected:
    RdmaNic();  // for serialization only
    ~RdmaNic() {}

    class Handlers : public Interfaces::StandardMem::RequestHandler {
      public:
        friend class RdmaNic;

        Handlers( RdmaNic* nic, SST::Output* out) : Interfaces::StandardMem::RequestHandler(out), m_nic(nic) {
    		std::ostringstream tmp;
			tmp << "@t:" << nic->m_nicId << ":RdmaNic::Handlers::@p():@l ";
    		m_prefix = tmp.str();
		}
        virtual ~Handlers() {}
        virtual void handle(Interfaces::StandardMem::Write* req) override {
			m_nic->m_mmioWriteFunc(req);
		}
        virtual void handle(Interfaces::StandardMem::WriteResp* req) override {
			m_nic->writeResp( req );
		}
        virtual void handle(Interfaces::StandardMem::Read*) override {
			assert(0);
		}
        virtual void handle(Interfaces::StandardMem::ReadResp* req) override {
			m_nic->readResp( req );
		}

        RdmaNic* m_nic;
		std::string m_prefix;
		const char* prefix() { return  m_prefix.c_str(); }
    };

	Handlers* m_handlers;


  private:

	#include "rdmaNicMemRequest.h"
	#include "rdmaNicMemRequestQ.h"

    MemRequestQ* m_memReqQ; 
    int m_tailWriteQnum;
    int m_respQueueMemChannel;
    int m_dmaMemChannel;

    struct NicCmdQueueInfo { 
        NicCmdQueueInfo( ){} 
        NicCmdQueueInfo( uint64_t  tailAddr ) :
            tailAddr(tailAddr), localTailIndex(0) {}

        uint64_t  tailAddr;
        uint32_t  localTailIndex;
    };
    std::vector<NicCmdQueueInfo> m_nicCmdQueueV;

    Output dbg;
    Output out;

    StandardMem* m_mmioLink; // mmioLink to memHierarchy
    StandardMem* m_dmaLink; // dmaLink to memHierarchy
    uint64_t m_ioBaseAddr;
    int m_pesPerNode;
    size_t m_perPeReqQueueMemSize;
	size_t m_perPeTotalMemSize;
	std::vector<int> m_cmdBytesWritten;
    std::vector<uint8_t> m_reqQueueBacking;
	std::vector< std::vector<QueueIndex> > m_compQueuesBacking;
#if 0 
    bool m_clockLink;            // Flag - should we call clock() on this link or not
#endif
    size_t m_cacheLineSize;
    uint64_t m_hostQueueBaseAddr;

	struct SetupInfo {
		SetupInfo() : offset(0) {}
		HostQueueInfo hostInfo;
		NicQueueInfo  nicInfo;
		int           offset;
	};

    struct ThreadInfo {
		ThreadInfo() {}
		SetupInfo setupInfo;

		struct CompTailInfo {
			CompTailInfo() : info(NUM_COMP_Q,std::vector<int>(RDMA_COMP_Q_SIZE, 0) )  {}
			
	 		std::vector< std::vector<int> > info;
			int read(int qNum, int qSlot ) { return info[qNum][qSlot];}
			//Addr_t getAddr(int qNum, int qSlot ) { return &info[qNum][qSlot] - this->comTailInfo;}
		} compTailInfo;
    };

    std::vector<ThreadInfo> m_threadInfo;

	void mmioWriteSetup( StandardMem::Write* );
	void mmioWrite( StandardMem::Write* );
	void writeResp( StandardMem::WriteResp* );
	void readResp( StandardMem::ReadResp* );

	std::function<void( StandardMem::Write* )> m_mmioWriteFunc;
	void handleEvent( StandardMem::Request *req ) {
		req->handle(m_handlers);
	}

    virtual bool clock( SST::Cycle_t );
    void processThreadCmdQs();
    void sendRespToHost( Addr_t, NicResp&,int thread );
	void writeCompletionToHost(int thread, int cqId, RdmaCompletion& comp );


    TimeConverter *m_clockTC;
    Clock::HandlerBase *m_clockHandler;

    Interfaces::SimpleNetwork*     m_linkControl;

	// Size of the NIC command Q
    int m_cmdQSize;

	int m_netPktMtuLen;

	struct StreamHdr {
		enum Type { Msg=1, Write, ReadReq, ReadResp, Barrier } type;	
		int seqLen;
		int payloadLength;
		union {
			RecvQueueKey msgKey;	
			struct {
				MemRgnKey memRgnKey;
				int offset;
				int readRespKey;
				int readLength;
			} rdma;
		} data; 
	};
	class CompletionQueue {
	  public:
		CompletionQueue( NicCmd * cmd) : m_cmd(cmd), m_headIndex(0) {}
		NicCmd& cmd() { return *m_cmd; }	
		int headIndex() { return m_headIndex; }
		void incHeadIndex() { 
			++m_headIndex; 
			m_headIndex %= m_cmd->data.createCQ.num;
		};
	  private:
		int m_headIndex;
		NicCmd* m_cmd;	
	};

	#include "rdmaNicBarrier.h"
	#include "rdmaNicNetworkQueue.h"
	#include "rdmaNicCmds.h"
	#include "rdmaNicSendEntry.h"
	#include "rdmaNicSendEngine.h"
	#include "rdmaNicRecvEngine.h"

	bool processNicCmd( NicCmdEntry * );
	bool processRdmaSend( NicCmdEntry * );

	int m_streamId;
	int m_numVC;
    int m_nicId;
    int m_numNodes;
	NetworkQueue* m_networkQ;
	RecvEngine* m_recvEngine;
	SendEngine* m_sendEngine;
	bool m_useDmaCache;

	Barrier* m_barrier;

	NicCmdEntry* m_activeNicCmd;
	// place for command from the host as they are written into MMIO space
	// this is used to preserve order
	std::queue< NicCmdEntry* > m_nicCmdQ; 

	std::map< int,CompletionQueue*> m_compQueueMap;

	int m_nextCqId;

	int getNetPktMtuLen() { return m_netPktMtuLen; }

	Addr_t calcCompQueueTailAddress( int thread, int cqId ) {
		Addr_t addr = m_ioBaseAddr + m_perPeTotalMemSize * thread; 
		addr += m_perPeReqQueueMemSize;
		addr += sizeof( QueueIndex ) * cqId;
		return addr;
	}

	int readCompQueueTailIndex( int thread, int cqId ) {
		return m_compQueuesBacking[thread][cqId];
	}

	StreamId generateStreamId() {
		return m_streamId++;
	}

	std::string getCmdStr( int type ) {
		switch ( type ) {
		case RdmaSend: return "RdmaSend";
		case RdmaRecv: return "RdmaRecv";
		case RdmaFini: return "RdmaFini";
		case RdmaCreateRQ: return "RdmaCreateRQ";
		case RdmaCreateCQ: return "RdmaCreateCQ";
		case RdmaMemRgnReg: return "RdmaMemRgnReg";
		case RdmaMemRgnUnreg: return "RdmaMemRgnUnreg";
		case RdmaMemWrite: return "RdmaMemWrite";
		case RdmaMemRead: return "RdmaMemRead";
		case RdmaBarrier: return "RdmaBarrier";
		default: return "Unknown RDMA command";
		}
	}
	NicCmdEntry* createNewCmd( RdmaNic& nic, int thread, NicCmd* cmd ) {
		dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"%s\n",getCmdStr(cmd->type).c_str());
		switch ( cmd->type ) {
	  	case RdmaSend:
			return new RdmaSendCmd( nic, thread, cmd );
	  	case RdmaRecv:
			return new RdmaRecvCmd( nic, thread, cmd );
	  	case RdmaFini:
			return new RdmaFiniCmd( nic, thread, cmd );
	  	case RdmaCreateRQ:
			return new RdmaCreateRQ_Cmd( nic, thread, cmd );
	  	case RdmaCreateCQ:
			return new RdmaCreateCQ_Cmd( nic, thread, cmd );
		case RdmaMemRgnReg:
			return new RdmaMemRgnRegCmd( nic, thread, cmd );
		case RdmaMemRgnUnreg:
			return new RdmaMemRgnUnregCmd( nic, thread, cmd );
	  	case RdmaMemWrite:
			return new RdmaMemWriteCmd( nic, thread, cmd );
	  	case RdmaMemRead:
			return new RdmaMemReadCmd( nic, thread, cmd );
	  	case RdmaBarrier:
			return new RdmaBarrierCmd( nic, thread, cmd );
		}
		dbg.output(CALL_INFO_LONG,"Error: thread=%d %d\n",thread,cmd->type);
		assert(0);
	}

};

static inline std::string getDataStr( std::vector<uint8_t>& data, size_t len = 0 ) {
	if ( len == 0 ) {
		len = data.size();
	}
    std::ostringstream tmp(std::ostringstream::ate);
        tmp << std::hex;
        for ( auto i = 0; i < len; i++ ) {
            tmp << "0x" << (int) data.at(i)  << ",";
        }
    return tmp.str();
}

}
}

#endif
