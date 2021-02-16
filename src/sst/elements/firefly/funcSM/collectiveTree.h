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

#ifndef COMPONENTS_FIREFLY_FUNCSM_COLLECTIVE_H
#define COMPONENTS_FIREFLY_FUNCSM_COLLECTIVE_H

#include "funcSM/api.h"
#include "funcSM/event.h"
#include "ctrlMsg.h"

namespace SST {
namespace Firefly {

class YYY {

  public:
    YYY( int degree, int myRank, int size, int root  ) :
        m_degree( degree ),
        m_myRank( myRank ),
        m_size( size ),
        m_numChildren(0),
        m_myVirtRank( myRank ),
        m_root( root ),
        m_parent( -1 )
    {
        if ( root > 0 ) {
            if ( root == m_myRank ) {
                m_myVirtRank = 0;
            } else if ( 0 == m_myRank )  {
                m_myVirtRank = root;
            }
        }

        for ( int i = 0; i < m_degree; i++ ) {
            if ( calcChild( i ) < m_size ) {
                ++m_numChildren;
            }
        }

        if ( m_myVirtRank > 0 ) {
            int tmp = m_myVirtRank % m_degree;
            tmp = 0 == tmp ? m_degree : tmp ;
            m_parent = (m_myVirtRank - tmp ) / m_degree;

            if ( m_parent == 0 ) {
                m_parent = m_root;
            } else if ( m_parent == m_root ) {
                m_parent = 0;
            }
        }
    }

    int myRank() { return m_myRank; }

    int size() { return m_size; }

    int parent() { return m_parent; }

    size_t numChildren() { return m_numChildren; }

    int calcChild( int i ) {
        int child = (m_myVirtRank * m_degree) + i + 1;
        // ummm, child can never be 0
        if ( child == 0 ) {
            child = m_root;
        }  else if ( child == m_root ) {
            child = 0;
        }
        return child;
    }

  private:

    int m_degree;
    int m_myRank;
    int m_size;
    int m_numChildren;
    int m_myVirtRank;
    int m_root;
    int m_parent;
};

#undef FOREACH_ENUM
#define FOREACH_ENUM(NAME) \
    NAME( WaitUp ) \
    NAME( SendUp ) \
    NAME( WaitDown ) \
    NAME( SendDown ) \
    NAME( Exit ) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

class CollectiveTreeFuncSM :  public FunctionSMInterface
{
    enum StateEnum {
        FOREACH_ENUM(GENERATE_ENUM)
    } m_state;

    static const char *m_enumName[];

    std::string stateName( StateEnum i ) {
        return m_enumName[i];
    }

    struct WaitUpState {
        WaitUpState() : count(0), state(Posting) {}
        unsigned int count;
        enum { Posting, Waiting, DoOp } state;
        void init() { state = Posting; count = 0; }
    };

    struct SendDownState {
        SendDownState() : count(0), state(Sending) {}
        unsigned int count;
        enum { Sending, Waiting } state;
        void init() { state = Sending; count = 0; }
    };

  public:
    CollectiveTreeFuncSM( SST::Params& params ) :
        FunctionSMInterface( params ),
        m_event( NULL ),
        m_seq( 0 ),
        m_vn( 0 )
    {
        m_smallCollectiveVN = params.find<int>( "smallCollectiveVN", 0);
        m_smallCollectiveSize = params.find<int>( "smallCollectiveSize", 0);
    }

    virtual void handleStartEvent( SST::Event*, Retval& );
    virtual void handleEnterEvent( Retval& );

  private:

    uint32_t    genTag() {
        return CtrlMsg::CollectiveTag | (m_seq & 0xffff);
    }

    CtrlMsg::API* proto() { return static_cast<CtrlMsg::API*>(m_proto); }

    WaitUpState         m_waitUpState;
    SendDownState       m_sendDownState;

    CollectiveStartEvent*   m_event;
    std::vector<CtrlMsg::CommReq>  m_recvReqV;
    std::vector<CtrlMsg::CommReq*>  m_recvReqV_ptrs;
    std::vector<CtrlMsg::CommReq>  m_sendReqV;
    std::vector<CtrlMsg::CommReq*>  m_sendReqV_ptrs;
    std::vector<void*>  m_bufV;
    size_t              m_bufLen;
    YYY*                m_yyy;
    int                 m_seq;

    int m_vn;
    int m_smallCollectiveVN;
    int m_smallCollectiveSize;
};

}
}

#endif
