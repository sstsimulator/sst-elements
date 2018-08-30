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


#ifndef _H_EMBER_SHMEM_GETV_EVENT
#define _H_EMBER_SHMEM_GETV_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

class EmberGetVShmemEvent : public EmberShmemEvent {

public:
	EmberGetVShmemEvent( Shmem::Interface& api, Output* output,
            Hermes::Value value, Hermes::Vaddr src, int pe, 
            EmberEventTimeStatistic* stat = NULL ) :
            EmberShmemEvent( api, output, stat ), 
            m_value(value), m_src(src),  m_pe(pe) {}
	~EmberGetVShmemEvent() {}

    std::string getName() { return "Malloc"; }

    void issue( uint64_t time, Callback callback ) {

        EmberEvent::issue( time );
        m_api.getv( m_value, m_src, m_pe, callback );
    }
private:
    Value  m_value;
    Hermes::Vaddr m_src;
    int m_pe;
};

}
}

#endif
