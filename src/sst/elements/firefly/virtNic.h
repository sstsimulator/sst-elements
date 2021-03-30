
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

#ifndef COMPONENTS_FIREFLY_VIRTNIC_H
#define COMPONENTS_FIREFLY_VIRTNIC_H

#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include "sst/elements/hermes/shmemapi.h"

#include "ioVec.h"

namespace SST {
namespace Firefly {

class NicRespEvent;
class NicShmemRespBaseEvent;

class VirtNic : public SST::SubComponent {

  public:
	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Firefly::VirtNic)

	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        VirtNic,
        "firefly",
        "VirtNic",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SST::Firefly::VirtNic
    )

    SST_ELI_DOCUMENT_PORTS(
        {"nic", "Connection to upper level nic", {}}
    )
    
    SST_ELI_DOCUMENT_PARAMS(
        {"verboseLevel","Sets the level of output","0"},
        {"maxNicQdepth","Sets maximum number of entries before blocking","32"},
        {"latPerSend_ns","Sets the time each send takes","2"},
        {"portName", "Sets the name of the port for the link", "nic"},
    )

  private:

    // Functor classes for handling callbacks
    template < typename argT >
    class HandlerBase {
      public:
        virtual bool operator()(argT) = 0;
        virtual ~HandlerBase() {}
    };

    template < typename T1, typename T2, typename T3, typename T4 >
    class HandlerBase4Args {
      public:
        virtual bool operator()( T1, T2, T3, T4 ) = 0;
        virtual ~HandlerBase4Args() {}
    };

    template < typename T1, typename T2 >
    class HandlerBase2Args {
      public:
        virtual bool operator()( T1, T2 ) = 0;
        virtual ~HandlerBase2Args() {}
    };

  public:

    typedef std::function<void(Hermes::Value&)> CallbackV;
    typedef std::function<void()> Callback;

    template <typename classT, typename argT >
    class Handler : public HandlerBase<argT> {
      private:
        typedef bool (classT::*PtrMember)(argT);
        classT* object;
        const PtrMember member;
        argT data;

      public:
        Handler( classT* const object, PtrMember member ) :
            object(object),
            member(member)
        {}

        bool operator()(argT data) {
            return (object->*member)(data);
        }
    };

    template <typename classT, typename T1, typename T2 >
    class Handler2Args : public HandlerBase2Args< T1, T2 > {
      private:
        typedef bool (classT::*PtrMember)( T1, T2 );
        classT* object;
        const PtrMember member;

      public:
        Handler2Args( classT* const object, PtrMember member ) :
            object(object),
            member(member)
        {}

        bool operator()( T1 arg1, T2 arg2) {
            return (object->*member)( arg1, arg2);
        }
    };

    template <typename classT, typename T1, typename T2,
                                        typename T3, typename T4 >
    class Handler4Args : public HandlerBase4Args< T1, T2, T3, T4 > {
      private:
        typedef bool (classT::*PtrMember)( T1, T2, T3, T4 );
        classT* object;
        const PtrMember member;

      public:
        Handler4Args( classT* const object, PtrMember member ) :
            object(object),
            member(member)
        {}

        bool operator()( T1 arg1, T2 arg2, T3 arg3, T4 arg4) {
            return (object->*member)( arg1, arg2, arg3, arg4 );
        }
    };


    VirtNic( ComponentId_t id, Params&);
    ~VirtNic();
    void init( unsigned int );

	int getNumCores() {
		return m_numCores;
	}

	int getCoreId() {
		return m_coreId;
	}

    int getRealNodeId() {
		return m_realNicId;
    }

    int getNodeId() {
		return m_realNicId * m_numCores + m_coreId;
    }

    bool isLocal( int nodeId, int nicsPerNode ) {
        return ( (nodeId / (m_numCores * nicsPerNode) ) == (m_realNicId / nicsPerNode) );
    }

	int calcCoreId( int nodeId, int nicsPerNode = 1) {
		if ( -1 == nodeId ) {
			return -1;
		} else if ( nodeId & (1<<31) )  {
			return 0;
		} else {
			return nodeId % (m_numCores * nicsPerNode);
		}
	}

    bool canDmaSend();
    bool canDmaRecv();

    void dmaRecv( int src, int tag, std::vector<IoVec>& vec, void* key );
    void pioSend( int vn, int dest, int tag, std::vector<IoVec>& vec, void* key  );
    void get( int node, int tag, std::vector<IoVec>& vec, void* key );
    void regMem( int node, int tag, std::vector<IoVec>& vec, void *key );

    void shmemInit( Hermes::Vaddr, Callback );
    void shmemFence( Callback );
    void shmemRegMem( Hermes::MemAddr&, Hermes::Vaddr, size_t len, Callback );
    void shmemWait( Hermes::Vaddr dest, Hermes::Shmem::WaitOp, Hermes::Value&, Callback );
    void shmemPutv( int node, Hermes::Vaddr dest, Hermes::Value& );
    void shmemGetv( int node, Hermes::Vaddr src, Hermes::Value::Type, CallbackV );
    void shmemPut( int node, Hermes::Vaddr dest, Hermes::Vaddr src, size_t len, Callback );
    void shmemGet( int node, Hermes::Vaddr dest, Hermes::Vaddr src, size_t len, Callback );
    void shmemPutOp( int node, Hermes::Vaddr dest, Hermes::Vaddr src, size_t len, Hermes::Shmem::ReduOp, Hermes::Value::Type, Callback );
    void shmemSwap( int node, Hermes::Vaddr dest, Hermes::Value& value, CallbackV );
    void shmemCswap( int node, Hermes::Vaddr dest, Hermes::Value& cond, Hermes::Value& value, CallbackV );
    void shmemAdd( int node, Hermes::Vaddr dest, Hermes::Value& );
    void shmemFadd( int node, Hermes::Vaddr dest, Hermes::Value&, CallbackV );

    void setNotifyOnRecvDmaDone(
        VirtNic::HandlerBase4Args<int,int,size_t,void*>* functor);
    void setNotifyOnSendPioDone(VirtNic::HandlerBase<void*>* functor);
    void setNotifyOnGetDone(VirtNic::HandlerBase<void*>* functor);
    void setNotifyNeedRecv( VirtNic::HandlerBase2Args<int,size_t>* functor);

    void notifyGetDone( void* key );
    void notifySendPioDone( void* key );
    void notifyRecvDmaDone( int src, int tag, size_t len, void* key );
    void notifyNeedRecv( int src, int tag, size_t length );

    bool isBlocked() {
		m_dbg.debug(CALL_INFO,2,0,"%d %d\n", m_curNicQdepth, m_maxNicQdepth);
        return m_curNicQdepth == m_maxNicQdepth;
    }

    void setBlockedCallback( Callback callback ) {
		m_dbg.debug(CALL_INFO,2,0,"\n");
        assert( ! m_blockedCallback );
        m_blockedCallback = callback;
    }

  private:

	SimTime_t m_nextTimeSlot;
	uint32_t m_latPerSend_ns;
	SimTime_t calcDelay() {
		SimTime_t curTime = Simulation::getSimulation()->getCurrentSimCycle()/1000;

		SimTime_t ret = curTime < m_nextTimeSlot ? m_nextTimeSlot - curTime: 0;	
		m_dbg.debug(CALL_INFO,2,0,"curTime_ns=%" PRIu64 " delay_ns=%" PRIu64"\n",curTime,ret);
		m_nextTimeSlot += m_latPerSend_ns;
		return ret;
	}

    void sendCmd( SimTime_t delay ,Event* ev) {
		m_dbg.debug(CALL_INFO,2,0,"%d %d\n", m_curNicQdepth, m_maxNicQdepth);
        assert( m_curNicQdepth < m_maxNicQdepth );
        ++m_curNicQdepth;
        m_toNicLink->send( delay, ev );
    }

	int calcRealNicId( int nodeId ) {
		if ( -1 == nodeId ) {
			return -1;
		} else if ( nodeId & (1<<31) )  {
			return nodeId & ~(1<<31);
		} else {
			return nodeId / m_numCores;
		}
	}
	int calcNodeId( int nic, int vNic ) {
        return nic * m_numCores + vNic;
    }

    void handleEvent( Event * );
    void handleMsgEvent( NicRespEvent * );
    void handleShmemEvent( NicShmemRespBaseEvent * );

    int         m_realNicId;
    int         m_coreId;
    int         m_numCores;
    Output      m_dbg;
    Link*       m_toNicLink;

    VirtNic::HandlerBase<void*>* m_notifyGetDone;
    VirtNic::HandlerBase<void*>* m_notifyPutDone;
    VirtNic::HandlerBase<void*>* m_notifySendPioDone;
    VirtNic::HandlerBase<void*>* m_notifySendDmaDone;
    VirtNic::HandlerBase4Args<int, int, size_t, void*>* m_notifyRecvDmaDone;
    VirtNic::HandlerBase2Args<int, size_t>* m_notifyNeedRecv;
    int m_maxNicQdepth;
    int m_curNicQdepth;
    Callback m_blockedCallback;
};

}
}

#endif
