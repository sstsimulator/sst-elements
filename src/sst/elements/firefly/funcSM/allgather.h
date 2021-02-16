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

#ifndef COMPONENTS_FIREFLY_FUNCSM_ALLGATHER_H
#define COMPONENTS_FIREFLY_FUNCSM_ALLGATHER_H

#include "funcSM/api.h"
#include "funcSM/event.h"
#include "ctrlMsg.h"
#include "info.h"

namespace SST {
namespace Firefly {

#undef FOREACH_ENUM

#define FOREACH_ENUM(NAME) \
    NAME(Setup) \
    NAME(WaitForStartMsg) \
    NAME(SendData) \
    NAME(WaitRecvData) \
    NAME(Exit) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

class AllgatherFuncSM :  public FunctionSMInterface
{
  public:
    SST_ELI_REGISTER_MODULE(
        AllgatherFuncSM,
        "firefly",
        "Allgather",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )

  private:
    enum StateEnum {
        FOREACH_ENUM(GENERATE_ENUM)
    } m_state;

    struct SetupState {
        SetupState() : count(0), state( PostStartMsgRecv ),
                offset(1), stage(0) {}
        void init() {
            count = 0;
            state = PostStartMsgRecv;
            offset = 1;
            stage = 0;
        }

        unsigned int count;
        enum { PostStartMsgRecv, PostStageRecv, SendStartMsg} state;
        int offset;
        unsigned int stage;
    };

  public:
    AllgatherFuncSM( SST::Params& params );

    virtual void handleStartEvent( SST::Event*, Retval& );
    virtual void handleEnterEvent( Retval& );

    virtual std::string protocolName() { return "CtrlMsgProtocol"; }

  private:

    bool setup( Retval& );
    void initIoVec(std::vector<IoVec>& ioVec, int startChunk, int numChunks, bool backed );

    std::string stateName( StateEnum i ) { return m_enumName[i]; }

    long mod( long a, long b ) { return (a % b + b) % b; }

    uint32_t genTag() {
        return CtrlMsg::AllgatherTag | (( m_seq & 0xff) << 8 );
    }

    int calcSrc ( int offset) {
        int src = ( m_rank - offset );
        return  src < 0 ? m_size + src : src;
    }

    unsigned char* chunkPtr( int rank ) {
        unsigned char* ptr = (unsigned char*) m_event->recvbuf.getBacking();
        if ( m_event->recvcntPtr ) {
            ptr += ((int*)m_event->displsPtr)[rank];
        } else {
            ptr += rank * chunkSize( rank );
        }
        m_dbg.debug(CALL_INFO,2,0,"rank %d, ptr %p\n", rank, ptr);

        return ptr;
    }

    size_t  chunkSize( int rank ) {
        size_t size;
		int count;
        if ( m_event->recvcntPtr ) {
			count = ((int*)m_event->recvcntPtr)[rank];
        } else {
			count = m_event->recvcnt;
        }
        size = m_info->sizeofDataType( m_event->recvtype ) * count;
        m_dbg.debug(CALL_INFO,2,0,"rank %d, size %lu count %d\n",rank,size,count);
        return size;
    }

    CtrlMsg::API* proto() { return static_cast<CtrlMsg::API*>(m_proto); }

    std::vector<CtrlMsg::CommReq>  m_recvReqV;
    CtrlMsg::CommReq    m_recvReq;
    GatherStartEvent*   m_event;
    int                 m_seq;
    SetupState          m_setupState;
    std::vector<int>    m_numChunks;
    std::vector<int>    m_sendStartChunk;
    std::vector<int>    m_dest;
    int                 m_rank;
    int                 m_size;
    unsigned int        m_currentStage;
    static const char*  m_enumName[];

    int m_smallCollectiveVN;
    int m_smallCollectiveSize;
};

}
}

#endif
