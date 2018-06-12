// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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

#undef FOREACH_ENUM

#define FOREACH_ENUM(NAME) \
    NAME( PostRecv ) \
    NAME( Send ) \
    NAME( WaitRecv ) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

class AlltoallvFuncSM :  public FunctionSMInterface
{
  public:
    SST_ELI_REGISTER_MODULE(
        AlltoallvFuncSM,
        "firefly",
        "Alltoallv",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )
  private:

    enum StateEnum {
         FOREACH_ENUM(GENERATE_ENUM)
    } m_state;

    static const char *m_enumName[];

    std::string stateName( StateEnum i ) {
        return m_enumName[i];
    }

  public:
    AlltoallvFuncSM( SST::Params& params ) :
        FunctionSMInterface( params ),
        m_event( NULL ),
        m_seq( 0 )
    { 
    }


    virtual void handleStartEvent( SST::Event*, Retval& );
    virtual void handleEnterEvent( Retval& );

    virtual std::string protocolName() { return "CtrlMsgProtocol"; }

  private:

    uint32_t    genTag() {
        return CtrlMsg::AlltoallvTag | (( m_seq & 0xff) << 8 );
    }

    unsigned char* sendChunkPtr( MP::RankID rank ) {
        unsigned char* ptr = (unsigned char*) m_event->sendbuf.getBacking();
        if ( ! ptr ) return NULL;
        if ( m_event->sendcnts ) {
            ptr += ((int*)m_event->senddispls)[rank];
        } else {
            ptr += rank * sendChunkSize( rank );
        }
        m_dbg.debug(CALL_INFO,2,0,"rank %d, buf %p, ptr %p\n", rank, 
                                    &m_event->sendbuf,ptr);

        return ptr;
    }

    size_t  sendChunkSize( MP::RankID rank ) {
        size_t size;
        if ( m_event->sendcnts ) {
            size = m_info->sizeofDataType( m_event->sendtype ) *
                        ((int*)m_event->sendcnts)[rank];
        } else {
            size = m_info->sizeofDataType( m_event->sendtype ) *
                                                m_event->sendcnt;
        }
        m_dbg.debug(CALL_INFO,2,0,"rank %d, size %lu\n",rank,size);
        return size;
    }

    unsigned char* recvChunkPtr( MP::RankID rank ) {
        unsigned char* ptr = (unsigned char*) m_event->recvbuf.getBacking();
        if ( ! ptr ) return NULL;
        if ( m_event->recvcnts ) {
            ptr += ((int*)m_event->recvdispls)[rank];
        } else {
            ptr += rank * recvChunkSize( rank );
        }
        m_dbg.debug(CALL_INFO,2,0,"rank %d, buf %p, ptr %p\n", rank, 
                    &m_event->recvbuf, ptr);

        return ptr;
    }

    size_t  recvChunkSize( MP::RankID rank ) {
        size_t size;
        if ( m_event->recvcnts ) {
            size = m_info->sizeofDataType( m_event->recvtype ) *
                        ((int*)m_event->recvcnts)[rank];
        } else {
            size = m_info->sizeofDataType( m_event->recvtype ) *
                                                m_event->recvcnt;
        }
        m_dbg.debug(CALL_INFO,2,0,"rank %d, size %lu\n",rank,size);
        return size;
    }

    CtrlMsg::API* proto() { return static_cast<CtrlMsg::API*>(m_proto); }

    AlltoallStartEvent* m_event;
    CtrlMsg::CommReq    m_recvReq; 
    unsigned int        m_count; 
    int                 m_seq;
    unsigned int        m_size;
    MP::RankID          m_rank;
};
        
}
}

#endif
