// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_CTRL_MSG_COMM_REQ_H
#define COMPONENTS_FIREFLY_CTRL_MSG_COMM_REQ_H


#if 0
#include "ctrlMsg.h"
#include "ioVec.h"
#include "latencyMod.h"
#include "mem.h"
#include "memory.h"
#include "msgTiming.h"
#endif

namespace SST {
namespace Firefly {
namespace CtrlMsg {

typedef unsigned short key_t;

struct MatchHdr {
    uint32_t count; 
    uint32_t dtypeSize;
    MP::RankID rank;
    MP::Communicator group;
    uint64_t    tag;
    key_t       key;      
};

class _CommReq : public MP::MessageRequestBase {
  public:

    enum Type { Recv, Send, Isend, Irecv };

    _CommReq( Type type, std::vector<IoVec>& _ioVec, 
        unsigned int dtypeSize, MP::RankID rank, uint32_t tag,
        MP::Communicator group ) : 
        m_type( type ),
        m_ioVec( _ioVec ),
        m_resp( NULL ),
        m_done( false ),
        m_destRank( MP::AnySrc ),
        m_ignore( 0 ),
        m_isMine( true ),
        m_finiDelay_ns( 0 )
    {
        m_hdr.count = getLength() / dtypeSize;
        m_hdr.dtypeSize = dtypeSize; 

        if ( m_type == Recv || m_type == Irecv ) {
            m_hdr.rank = rank;
        } else {
            m_destRank = rank;
        }

        m_hdr.tag = tag;
        m_hdr.group = group;
    }

    _CommReq( Type type, const Hermes::MemAddr& buf, uint32_t count,
        unsigned int dtypeSize, MP::RankID rank, uint32_t tag, 
        MP::Communicator group, MP::MessageResponse* resp = NULL ) :
        m_type( type ),
        m_resp( resp ),
        m_done( false ),
        m_destRank( MP::AnySrc ),
        m_ignore( 0 ),
        m_isMine( true ),
        m_finiDelay_ns( 0 )
    { 
        m_hdr.count = count;
        m_hdr.dtypeSize = dtypeSize; 

        if ( m_type == Recv || m_type == Irecv ) {
            m_hdr.rank = rank;
            if ( MP::AnyTag == tag  ) {
                m_ignore = 0xffffffff;
            }
        } else {
            m_destRank = rank;
        }

        m_hdr.tag = tag;
        m_hdr.group = group;
        m_ioVec.resize( 1 );
        m_ioVec[0].addr = buf;
        m_ioVec[0].len = dtypeSize * count;
    }
    ~_CommReq() {
    }

    bool isBlocking() {
        return m_type == Recv || m_type == Send;
    }

    uint64_t ignore() { return m_ignore; }
    void setSrcRank( MP::RankID rank ) {
        m_hdr.rank = rank;
    }

    MatchHdr& hdr() { return m_hdr; }
    
    std::vector<IoVec>& ioVec() { 
        assert( ! m_ioVec.empty() );
        return m_ioVec; 
    }

    bool isDone() { return m_done; }
    void setDone(int delay = 0 ) { 
        m_finiDelay_ns = delay;
        m_done = true; 
    }

    MP::MessageResponse* getResp(  ) {
        return &m_matchInfo;
    }

    void setResp( uint32_t tag, MP::RankID src, uint32_t count )
    {
        m_matchInfo.tag = tag;
        m_matchInfo.src = src;
        m_matchInfo.count = count;

        if ( m_resp ) {
            *m_resp = m_matchInfo;
        }
    }

    MP::RankID getDestRank() { return m_destRank; }
    MP::Communicator getGroup() { return m_hdr.group; }
    
    size_t getLength( ) {
        size_t length = 0;
        for ( size_t i = 0; i < m_ioVec.size(); i++ ) {
            length += m_ioVec[i].len;
        }
        return length;
    }

    bool isMine( ) {
        return m_isMine;
    }
    int  getFiniDelay() { return m_finiDelay_ns; }

    bool isSend() {
        if ( m_type == Isend  || m_type == Send ) {
            return true;
        } else {
            return false;
        }
    }

    // need to save info for the long protocol ack
    int m_ackKey;
    int m_ackNid;

  private:

    MatchHdr            m_hdr; 
    Type                m_type;
    std::vector<IoVec>  m_ioVec;
    MP::MessageResponse* m_resp;
    bool                m_done;
    MP::RankID      m_destRank;
    MP::MessageResponse  m_matchInfo;
    uint64_t            m_ignore;
    bool                m_isMine; 
    int                 m_finiDelay_ns;
};

}
}
}

#endif
