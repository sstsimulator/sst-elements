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


#ifndef _H_EMBER_SCATTERV_EV
#define _H_EMBER_SCATTERV_EV

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberScattervEvent : public EmberMPIEvent {
public:
    EmberScattervEvent( MP::Interface& api, Output* output, EmberEventTimeStatistic* stat,

            const Hermes::MemAddr& sendData, int* sendCnts, int* sendDsp, PayloadDataType senddtype,
			const Hermes::MemAddr& recvData, uint32_t recvCnt, PayloadDataType recvdtype,
            int root, Communicator group ) :
        EmberMPIEvent( api, output, stat ),
        m_sendData(sendData),
        m_sendCnts(sendCnts),
        m_sendDsp(sendDsp),
        m_sendDtype(senddtype),
        m_recvData(recvData),
        m_recvCnt(recvCnt),
        m_recvDtype( recvdtype ),
        m_root( root ),
        m_group(group)
    {}

	~EmberScattervEvent() {}

    std::string getName() { return "Scatterv"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.scatterv( m_sendData, m_sendCnts, m_sendDsp, m_sendDtype, m_recvData, m_recvCnt, m_recvDtype, m_root, m_group, functor );
    }

private:
    Hermes::MemAddr     m_sendData;
    int*                m_sendCnts;
    int*                m_sendDsp;
    PayloadDataType     m_sendDtype;

    Hermes::MemAddr     m_recvData;
    uint32_t            m_recvCnt;
    PayloadDataType     m_recvDtype;

    int                 m_root;
    Communicator        m_group;
};

}
}

#endif
