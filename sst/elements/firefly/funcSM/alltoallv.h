// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_ALLTOALLV_H
#define COMPONENTS_FIREFLY_FUNCSM_ALLTOALLV_H

#include "funcSM/api.h"
#include "funcSM/event.h"
#include "info.h"
#include "ctrlMsg.h"

namespace SST {
namespace Firefly {

class AlltoallvFuncSM :  public FunctionSMInterface
{
    static const int AlltoallvTag = 0xf0030000;

  public:
    AlltoallvFuncSM( int verboseLevel, Output::output_location_t loc,
            Info* info, ProtocolAPI*, SST::Link* );

    virtual void handleStartEvent( SST::Event* );
    virtual void handleEnterEvent( SST::Event*);
    virtual void handleSelfEvent( SST::Event* );

    virtual const char* name() {
       return "Alltoallv"; 
    }

  private:

    uint32_t    genTag() {
        return AlltoallvTag | (( m_seq & 0xff) << 8 );
    }


    unsigned char* sendChunkPtr( int rank ) {
        unsigned char* ptr = (unsigned char*) m_event->sendbuf;
        if ( m_event->sendcnts ) {
            ptr += ((int*)m_event->senddispls)[rank];
        } else {
            ptr += rank * sendChunkSize( rank );
        }
        m_dbg.verbose(CALL_INFO,2,0,"rank %d, buf %p, ptr %p\n", rank, 
                                    m_event->sendbuf,ptr);

        return ptr;
    }

    size_t  sendChunkSize( int rank ) {
        size_t size;
        if ( m_event->sendcnts ) {
            size = m_info->sizeofDataType( m_event->sendtype ) *
                        ((int*)m_event->sendcnts)[rank];
        } else {
            size = m_info->sizeofDataType( m_event->sendtype ) *
                                                m_event->sendcnt;
        }
        m_dbg.verbose(CALL_INFO,2,0,"rank %d, size %lu\n",rank,size);
        return size;
    }

    unsigned char* recvChunkPtr( int rank ) {
        unsigned char* ptr = (unsigned char*) m_event->recvbuf;
        if ( m_event->recvcnts ) {
            ptr += ((int*)m_event->recvdispls)[rank];
        } else {
            ptr += rank * recvChunkSize( rank );
        }
        m_dbg.verbose(CALL_INFO,2,0,"rank %d, buf %p, ptr %p\n", rank, 
                    m_event->recvbuf, ptr);

        return ptr;
    }

    size_t  recvChunkSize( int rank ) {
        size_t size;
        if ( m_event->recvcnts ) {
            size = m_info->sizeofDataType( m_event->recvtype ) *
                        ((int*)m_event->recvcnts)[rank];
        } else {
            size = m_info->sizeofDataType( m_event->recvtype ) *
                                                m_event->recvcnt;
        }
        m_dbg.verbose(CALL_INFO,2,0,"rank %d, size %lu\n",rank,size);
        return size;
    }

    SST::Link*          m_selfLink;
    CtrlMsg*            m_ctrlMsg;
    AlltoallStartEvent* m_event;
    bool                m_pending;
    CtrlMsg::CommReq    m_sendReq; 
    CtrlMsg::CommReq    m_recvReq; 
    unsigned int        m_count; 
    int                 m_seq;
    unsigned int        m_size;
    int                 m_rank;
    int                 m_delay;
    bool                m_test;
    bool                m_waitSend;
};
        
}
}

#endif
