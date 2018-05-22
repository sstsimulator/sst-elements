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


#ifndef _H_EMBER_SHMEM_NPES_EVENT
#define _H_EMBER_SHMEM_NPES_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

class EmberNPesShmemEvent : public EmberShmemEvent {

public:
	EmberNPesShmemEvent( Shmem::Interface& api, Output* output,
                    int* val, EmberEventTimeStatistic* stat = NULL ) :
            EmberShmemEvent( api, output, stat ), m_val(val) {}
	~EmberNPesShmemEvent() {}

    std::string getName() { return "NPes"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.n_pes( m_val, callback );
    }
private:
    int* m_val;
};

}
}

#endif
