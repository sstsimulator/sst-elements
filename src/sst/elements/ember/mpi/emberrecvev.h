// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_RECV_EVENT
#define _H_EMBER_RECV_EVENT

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberRecvEvent : public EmberMPIEvent {

  public:
	EmberRecvEvent( MP::Interface& api, Output* output,
                   EmberEventTimeStatistic* stat,
        Addr payload, uint32_t count, PayloadDataType dtype, RankID src, 
        uint32_t tag, Communicator group, MessageResponse* resp ) :
        EmberMPIEvent( api, output, stat ),
        m_payload(payload),
        m_count(count),
        m_dtype(dtype),
        m_src(src),
        m_tag(tag),
        m_group(group),
        m_resp(resp)       
    {}

    std::string getName() { return "Recv"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.recv( m_payload, m_count, m_dtype, m_src, m_tag,
                                                    m_group, m_resp, functor );
    }

	~EmberRecvEvent() {}

  private:
    Addr            m_payload;
    uint32_t        m_count;
    PayloadDataType m_dtype;
    RankID          m_src;
    uint32_t        m_tag;
    Communicator    m_group;

    MessageResponse* m_resp;
};

}
}

#endif
