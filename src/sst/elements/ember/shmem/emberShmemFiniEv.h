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


#ifndef _H_EMBER_SHMEM_FINI_EVENT
#define _H_EMBER_SHMEM_FINI_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

class EmberFiniShmemEvent : public EmberShmemEvent {

public:
	EmberFiniShmemEvent( Shmem::Interface& api, Output* output,
                    EmberEventTimeStatistic* stat = NULL ) :
            EmberShmemEvent( api, output, stat ){}
	~EmberFiniShmemEvent() {}

    std::string getName() { return "Fini"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.finalize( callback );
    }
};

}
}

#endif
