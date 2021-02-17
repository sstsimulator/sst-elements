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


#ifndef _H_EMBER_SHMEM_WAIT_EVENT
#define _H_EMBER_SHMEM_WAIT_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

class EmberWaitShmemEvent : public EmberShmemEvent {

public:
	EmberWaitShmemEvent( Shmem::Interface& api, Output* output,
            Hermes::Vaddr dest, Hermes::Shmem::WaitOp op, Hermes::Value value,
            EmberEventTimeStatistic* stat = NULL ) :
            EmberShmemEvent( api, output, stat ),
            m_dest(dest), m_op(op), m_value(value) {}
	~EmberWaitShmemEvent() {}

    std::string getName() { return "Wait"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.wait_until( m_dest, m_op, m_value, callback );
    }
private:
    Hermes::Vaddr m_dest;
    Hermes::Value m_value;
    Hermes::Shmem::WaitOp m_op;
};

}
}

#endif
