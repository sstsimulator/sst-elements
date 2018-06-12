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

#ifndef COMPONENTS_FIREFLY_FUNCSM_GATHERV_H
#define COMPONENTS_FIREFLY_FUNCSM_GATHERV_H

#include "funcSM/api.h"
#include "funcSM/event.h"
#include "ctrlMsg.h"

namespace SST {
namespace Firefly {

#undef FOREACH_ENUM

#define FOREACH_ENUM(NAME) \
    NAME(WaitUp) \
    NAME(SendUp) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

class QQQ {

  public:
    QQQ( int degree, int myRank, int size, int root  ) :
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

    void foo( int x, std::vector<int>& map ) {
        if ( m_root ) {
            if ( x == m_root ) {
                map.push_back(0);
            } else if ( 0 == x )  {
                map.push_back( m_root);
            } else { 
                map.push_back(x);
            }
        } else { 
            map.push_back(x);
        }

        for ( int i = 0; i < m_degree; i++ ) {
            int child = (x * m_degree) + i + 1; 
            if ( child < m_size ) {
                foo( child, map );
            }
        }
    }

    std::vector<int> getMap() {
        std::vector<int> map;
        foo( 0, map ); 
        return map;
    }

    int myRank() { return m_myRank; }
    int size() { return m_size; }

    int parent() { return m_parent; }

    size_t numChildren() { return m_numChildren; }

    int calcChild( int i ) {
        int child = (m_myVirtRank * m_degree) + i + 1;
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


class GathervFuncSM :  public FunctionSMInterface
{
  public:
    SST_ELI_REGISTER_MODULE(
        GathervFuncSM,
        "firefly",
        "Gatherv",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )

  private:
    enum StateEnum {
        FOREACH_ENUM(GENERATE_ENUM)
    } m_state;

    struct WaitUpState {
        WaitUpState() : state(PostSizeRecvs), count(0) {} 
        void init() {
            state = PostSizeRecvs;
            count = 0;
        }
        enum { PostSizeRecvs, WaitSizeRecvs, Setup, PostDataRecv, 
            SendSize, WaitDataRecv, DoRoot } state;
        unsigned int    count;
        size_t          len;
    };

    struct SendUpState {
        SendUpState() : state(SendSize) {}
        void init() {
            state = SendSize;
        }
        enum { SendSize, RecvGo, SendBody, SentBody } state;
    };

    SendUpState m_sendUpState;
    WaitUpState m_waitUpState;

  public:
    GathervFuncSM( SST::Params& params );

    virtual void handleStartEvent( SST::Event *e, Retval& );
    virtual void handleEnterEvent( Retval& );

    virtual std::string protocolName() { return "CtrlMsgProtocol"; }

  private:

    bool waitUp(Retval&);
    bool sendUp(Retval&);
    void doRoot();
    uint32_t    genTag( int i = 0 ) {
        return CtrlMsg::GathervTag | i << 8 | (m_seq & 0xff);
    } 

    CtrlMsg::API* proto() { return static_cast<CtrlMsg::API*>(m_proto); }

    GatherStartEvent*   m_event;
    QQQ*                m_qqq;
    std::vector<CtrlMsg::CommReq>  m_recvReqV;

    std::vector<int>    m_waitUpSize;
    std::vector<unsigned char>  m_recvBuf;
    int                 m_intBuf;
    int                 m_seq;
};
        
}
}

#endif
