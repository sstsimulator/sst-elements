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


#ifndef _H_EMBER_IRECV_EVENT
#define _H_EMBER_IRECV_EVENT

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberIRecvEvent : public EmberMPIEvent {

public:
	EmberIRecvEvent( MP::Interface& api, Output* output,
                    EmberEventTimeStatistic* stat,
            Addr payload, uint32_t count, PayloadDataType dtype, RankID dest,
            uint32_t tag, Communicator group, MessageRequest* req ) :
        EmberMPIEvent( api, output, stat ),
        m_payload(payload),
        m_count(count),
        m_dtype(dtype),
        m_dest(dest),
        m_tag(tag),
        m_group(group),
        m_req(req)
    {}

    ~EmberIRecvEvent() {}

    std::string getName() { return "Irecv"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.irecv( m_payload, m_count, m_dtype, m_dest, m_tag,
                                                    m_group, m_req, functor );
    }

protected:
    Addr            m_payload;
    uint32_t        m_count;
    PayloadDataType m_dtype;
    RankID          m_dest;
    uint32_t        m_tag;
    Communicator    m_group;
    MessageRequest* m_req;
};

}
}

#endif
