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


#ifndef _H_EMBER_SHMEM_MALLOC_EVENT
#define _H_EMBER_SHMEM_MALLOC_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

class EmberMallocShmemEvent : public EmberShmemEvent {

public:
	EmberMallocShmemEvent( Shmem::Interface& api, Output* output,
            Hermes::MemAddr* ptr, size_t val, bool backed,
            EmberEventTimeStatistic* stat = NULL ) :
            EmberShmemEvent( api, output, stat ), m_ptr(ptr), m_val(val), m_backed(backed) {}
	~EmberMallocShmemEvent() {}

    std::string getName() { return "Malloc"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.malloc( m_ptr, m_val, m_backed, callback );
    }
private:
	bool m_backed;
    Hermes::MemAddr* m_ptr;
    size_t m_val;
};

}
}

#endif
