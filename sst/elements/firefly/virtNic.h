
// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_VIRTNIC_H
#define COMPONENTS_FIREFLY_VIRTNIC_H

#include <sst/core/module.h>
#include <sst/core/output.h>
#include <sst/core/component.h>

#include "ioVec.h"

namespace SST {
namespace Firefly {

class VirtNic : public SST::Module {

    static const int coreShift = 21;
    static const int nidMask = (1 << coreShift) - 1;

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

    template < typename T1, typename T2, typename T3 >
    class HandlerBase3Args {
      public:
        virtual bool operator()( T1, T2, T3 ) = 0;
        virtual ~HandlerBase3Args() {}
    };

  public:

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

    template <typename classT, typename T1, typename T2, typename T3 >
    class Handler3Args : public HandlerBase3Args< T1, T2, T3 > {
      private:
        typedef bool (classT::*PtrMember)( T1, T2, T3 );
        classT* object;
        const PtrMember member;

      public:
        Handler3Args( classT* const object, PtrMember member ) :
            object(object),
            member(member)
        {}

        bool operator()( T1 arg1, T2 arg2, T3 arg3) {
            return (object->*member)( arg1, arg2, arg3);
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


    VirtNic( Component* owner, Params&);
    ~VirtNic();
    void init( unsigned int );

	int getNumCores() {
		return m_numCores;
	}

	int getCoreId() {
		return m_coreId;
	}

	int getRealNicId() {
		return m_realNicId;
	}

    int getVirtNicId() {
        return calcVirtNicId( m_realNicId, m_coreId );
    }

	int calcCoreId( int virtNicId ) {
		if ( -1 == virtNicId ) {
			return -1;
		} else {
			return virtNicId >> coreShift;
		}
	}

	int calcRealNicId( int virtNicId ) {
		if ( -1 == virtNicId ) {
			return -1;
		} else {
			return virtNicId & nidMask;
		}
	}

	int calcVirtNicId( int realNicId, int coreId ) {
        return (coreId << coreShift) | realNicId;
	}

    bool isLocal( int virtNicId ) {
        return ( calcRealNicId( virtNicId ) == m_realNicId );
    }

    bool canDmaSend();
    bool canDmaRecv();

    void dmaSend( int dest, int tag, std::vector<IoVec>& vec, void* key );
    void dmaRecv( int src, int tag, std::vector<IoVec>& vec, void* key );
    void pioSend( int dest, int tag, std::vector<IoVec>& vec, void* key );
    void get( int node, int tag, std::vector<IoVec>& vec, void* key );
    void put( int node, int tag, std::vector<IoVec>& vec, void* key );
    void regMem( int node, int tag, std::vector<IoVec>& vec, void *key );

    void setNotifyOnSendDmaDone(VirtNic::HandlerBase<void*>* functor);
    void setNotifyOnRecvDmaDone(
        VirtNic::HandlerBase4Args<int,int,size_t,void*>* functor);
    void setNotifyOnSendPioDone(VirtNic::HandlerBase<void*>* functor);
    void setNotifyOnGetDone(VirtNic::HandlerBase<void*>* functor);
    void setNotifyOnPutDone(VirtNic::HandlerBase<void*>* functor);
    void setNotifyNeedRecv( VirtNic::HandlerBase3Args<int,int,size_t>* functor);

    void notifyGetDone( void* key );
    void notifyPutDone( void* key );
    void notifySendPioDone( void* key );
    void notifySendDmaDone( void* key );
    void notifyRecvDmaDone( int src, int tag, size_t len, void* key );
    void notifyNeedRecv( int src, int tag, size_t length );

  private:

    void handleEvent( Event * );

    int         m_realNicId;
    int         m_coreId;
    int         m_numCores;
    Output      m_dbg;
    Link*       m_toNicLink;

    Output::output_location_t   m_dbg_loc;
    int                         m_dbg_level;

    VirtNic::HandlerBase<void*>* m_notifyGetDone; 
    VirtNic::HandlerBase<void*>* m_notifyPutDone; 
    VirtNic::HandlerBase<void*>* m_notifySendPioDone; 
    VirtNic::HandlerBase<void*>* m_notifySendDmaDone; 
    VirtNic::HandlerBase4Args<int, int, size_t, void*>* m_notifyRecvDmaDone; 
    VirtNic::HandlerBase3Args<int, int, size_t>* m_notifyNeedRecv;
};

}
}

#endif
