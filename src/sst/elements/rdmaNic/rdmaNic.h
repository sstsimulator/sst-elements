// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_SHMEM_NIC_H
#define MEMHIERARCHY_SHMEM_NIC_H

#include <list>
#include <queue>
#include <sst/core/sst_types.h>

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/componentExtension.h>

#include <sst/core/interfaces/stdMem.h>
#include <sst/core/interfaces/simpleNetwork.h>

typedef uint64_t Addr_t;

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

    SST_ELI_REGISTER_COMPONENT(RdmaNic, "rdmaNic", "nic", SST_ELI_ELEMENT_VERSION(1,0,0),
        "RDMA NIC, interfaces to a main memory model as memory and CPU", COMPONENT_CATEGORY_MEMORY)

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

    SST_ELI_DOCUMENT_PORTS({ "dma", "Connects the NIC to a cache for DMA", {} },
                           { "mmio", "Connects the NIC to MH", {} },
                           { "rtrLink", "Connects the NIC to the network", {} })


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

    StandardMem* m_mmioLink;
    StandardMem* m_dmaLink;
    uint64_t m_ioBaseAddr;


    template < typename CMD, typename COMP_INDEX>
    class Backing {

        int round_up(int num, int factor)
        {
            return num + factor - 1 - (num + factor - 1) % factor;
        }

      public:
        Backing( int numPe, int cmdQSize, int compQSize ) : peBacking( numPe, PE_backing( compQSize ) ), cmdQSize(cmdQSize), compQSize(compQSize)
        {
            assert( numPe );
            //printf("%s() numPe=%d cmdQSize=%d compQSize=%d\n",__func__,numPe, cmdQSize, compQSize);
        }

        int getNumPEs() { return peBacking.size(); }
        size_t getMemorySize() { return getPeMemorySize() * peBacking.size(); }

        size_t getCmdQSize()        { return cmdQSize; }
        size_t getCmdQueueMemSize() { return getCmdQSize() * sizeof(CMD); }

        size_t getCompInfoOffset()  { return round_up( getCmdQueueMemSize(), 4096 ); }
        size_t getCompInfoMemSize() { return sizeof(COMP_INDEX) * compQSize; }

        size_t getPeMemorySize() {
            size_t tmp = getCompInfoOffset();
            tmp += round_up( getCompInfoMemSize(), 4096);
            return tmp;
        }

        bool write( uint64_t offset, unsigned char* data, size_t length ) {
            auto retval = false;
            auto thread = calcThread( offset );
            auto& pe = peBacking[thread];

            //printf("%s() thread=%d offset=%#x length=%zu\n",__func__, thread, offset, length );

            offset = offset % getPeMemorySize();

            if ( offset + length <= getCmdQueueMemSize() ) {
                auto index = offset / getCmdQSize();

                //printf("%s() write cmd index=%d bytesWritten=%d\n",__func__, index, pe.bytesWritten );
                assert( index == pe.cmdIndex );

                pe.bytesWritten += length;
                offset = (offset % sizeof(NicCmd));
                auto ptr = ( (unsigned char*) &pe.cmd ) + offset;
                memcpy( ptr, data, length );
                retval = true;


            } if ( offset >= getCompInfoOffset() && ( offset + length <= getCompInfoOffset() + getCompInfoMemSize() ) ) {

                offset -= getCompInfoOffset();

                auto index = offset / sizeof( COMP_INDEX );

                memcpy( &pe.compQueuesTailIndex[index], data, length );
                //printf("write comp index=%d value=%d\n", index, pe.compQueuesTailIndex[index]);
                retval = true;
            }

            return retval;
        }

        bool isCmdReady( int thread ) {
            return peBacking[thread].bytesWritten == sizeof(CMD);
        }

        NicCmd* readCmd( int thread ) {
            //printf("%s()\n",__func__);
            auto& pe = peBacking[thread];
            pe.bytesWritten = 0;
            ++pe.cmdIndex;
            pe.cmdIndex %= getCmdQSize();
            return &pe.cmd;
        }

        uint64_t getCompQueueTailOffset( int thread, int cqId ) {
            uint64_t offset = (getPeMemorySize() * thread) + (cqId * sizeof( COMP_INDEX ) );
            //printf("%s() thread=%d cqId=%d\n",__func__,thread,cqId,offset);
            return offset;
        }
        int getCompQueueTailIndex( int thread, int cqId ) {
            auto& pe = peBacking[thread];

            //printf("%s() thread=%d cqId=%d index=%d\n",__func__,thread,cqId, pe.compQueuesTailIndex[cqId]);
            return pe.compQueuesTailIndex[cqId];
        }

      private:

        int calcThread( uint64_t addr ) {
            return addr / getPeMemorySize();
        }

        struct PE_backing {
            PE_backing( int compQSize) : compQueuesTailIndex( compQSize, 0), bytesWritten(0), cmdIndex(0) {}

            CMD  cmd;
            int  bytesWritten;

            // each PE has N Completion Queues, this vector holds the tail index for each
	        std::vector< COMP_INDEX > compQueuesTailIndex;

            int cmdIndex;
        };

        std::vector< PE_backing > peBacking;
        int cmdQSize;
        int compQSize;
    };

    uint64_t calcOffset( uint64_t addr ) {
        return addr - m_ioBaseAddr;
    }

    int calcThread( uint64_t addr ) {
        return ( addr - m_ioBaseAddr ) / m_backing->getPeMemorySize();
    }

    Backing< NicCmd, QueueIndex >* m_backing;

    struct HostInfo {
		HostInfo() : offset(0) {}
		HostQueueInfo   hostInfo;
		int             offset;
    };

    std::vector<HostInfo> m_hostInfo;

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
        ~CompletionQueue() { delete m_cmd; }

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

	Barrier* m_barrier;

	NicCmdEntry* m_activeNicCmd;
	// place for command from the host as they are written into MMIO space
	// this is used to preserve order
	std::queue< NicCmdEntry* > m_nicCmdQ;

	std::map< int,CompletionQueue*> m_compQueueMap;

	int m_nextCqId;

	int getNetPktMtuLen() { return m_netPktMtuLen; }

	Addr_t calcCompQueueTailAddress( int thread, int cqId ) {
        return m_backing->getCompQueueTailOffset( thread, cqId );
	}

	int readCompQueueTailIndex( int thread, int cqId ) {
        return m_backing->getCompQueueTailIndex( thread, cqId );
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
		case RdmaDestroyRQ: return "RdmaDestroyRQ";
		case RdmaCreateCQ: return "RdmaCreateCQ";
		case RdmaDestroyCQ: return "RdmaDestroyCQ";
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
	  	case RdmaDestroyRQ:
			return new RdmaDestroyRQ_Cmd( nic, thread, cmd );
	  	case RdmaCreateCQ:
			return new RdmaCreateCQ_Cmd( nic, thread, cmd );
	  	case RdmaDestroyCQ:
			return new RdmaDestroyCQ_Cmd( nic, thread, cmd );
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
        default:
		    dbg.output(CALL_INFO_LONG,"Error: thread=%d %d\n",thread,cmd->type);
		    assert(0);
		}
                return nullptr;
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
