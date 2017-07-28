// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SHMEM_MALLOC_EVENT
#define _H_EMBER_SHMEM_MALLOC_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

class EmberMallocShmemEvent : public EmberShmemEvent {

public:
	EmberMallocShmemEvent( Shmem::Interface& api, Output* output,
            Hermes::MemAddr* ptr, size_t val, 
            EmberEventTimeStatistic* stat = NULL ) :
            EmberShmemEvent( api, output, stat ), m_ptr(ptr), m_val(val) {}
	~EmberMallocShmemEvent() {}

    std::string getName() { return "Malloc"; }

    void issue( uint64_t time, MP::Functor* functor ) {

        EmberEvent::issue( time );
        m_api.malloc( m_ptr, m_val, functor );
    }
private:
    Hermes::MemAddr* m_ptr;
    size_t m_val;
};

}
}

#endif
