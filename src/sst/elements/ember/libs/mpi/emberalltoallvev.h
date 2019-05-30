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


#ifndef _H_EMBER_ALLTOALLV_EV
#define _H_EMBER_ALLTOALLV_EV

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberAlltoallvEvent : public EmberMPIEvent {
public:
    EmberAlltoallvEvent( MP::Interface& api, Output* output,
                        EmberEventTimeStatistic* stat,
        const Hermes::MemAddr& sendData,
        Addr sendCnts, Addr sendDsp, PayloadDataType senddtype,
        const Hermes::MemAddr& recvData,
        Addr recvCnts, Addr recvDsp, PayloadDataType recvdtype, 
        Communicator group ) :

        EmberMPIEvent( api, output, stat ),
        m_senddata(sendData),
        m_sendcnts(sendCnts),
        m_senddsp(sendDsp),
        m_senddtype(senddtype),
        m_recvdata(recvData),
        m_recvcnts(recvCnts),
        m_recvdsp(recvDsp),
        m_recvdtype(recvdtype),
        m_group(group)
    {}

	~EmberAlltoallvEvent() {}

    std::string getName() { return "Alltoallv"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.alltoallv( m_senddata, m_sendcnts, m_senddsp, m_senddtype,
                        m_recvdata, m_recvcnts, m_recvdsp, m_recvdtype,
                                                    m_group, functor );
    }

private:
    Hermes::MemAddr     m_senddata;
    Addr                m_sendcnts;
    Addr                m_senddsp;
    PayloadDataType     m_senddtype;
    Hermes::MemAddr     m_recvdata;
    Addr                m_recvcnts;
    Addr                m_recvdsp;
    PayloadDataType     m_recvdtype;
    Communicator        m_group;
};

}
}

#endif
