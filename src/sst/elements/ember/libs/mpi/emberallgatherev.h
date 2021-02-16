// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_ALLGATHER_EV
#define _H_EMBER_ALLGATHER_EV

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberAllgatherEvent : public EmberMPIEvent {
public:
    EmberAllgatherEvent( MP::Interface& api, Output* output,
                       EmberEventTimeStatistic* stat,
        const Hermes::MemAddr& sendData,
        int sendCnts, PayloadDataType senddtype,
        const Hermes::MemAddr& recvData,
        int recvCnts, PayloadDataType recvdtype,
        Communicator group ) :

        EmberMPIEvent( api, output, stat ),
        m_senddata(sendData),
        m_sendcnts(sendCnts),
        m_senddtype(senddtype),
        m_recvdata(recvData),
        m_recvcnts(recvCnts),
        m_recvdtype(recvdtype),
        m_group(group)
    {}

	~EmberAllgatherEvent() {}

    std::string getName() { return "Allgather"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.allgather( m_senddata, m_sendcnts, m_senddtype,
                        m_recvdata, m_recvcnts, m_recvdtype, m_group, functor );
    }

private:
    Hermes::MemAddr     m_senddata;
    int                 m_sendcnts;
    PayloadDataType     m_senddtype;
    Hermes::MemAddr     m_recvdata;
    int                 m_recvcnts;
    PayloadDataType     m_recvdtype;
    Communicator        m_group;
};

}
}

#endif
