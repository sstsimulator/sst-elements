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


#ifndef _H_EMBER_SHMEM_MYPE_EVENT
#define _H_EMBER_SHMEM_MYPE_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

class EmberMyPeShmemEvent : public EmberShmemEvent {

public:
	EmberMyPeShmemEvent( Shmem::Interface& api, Output* output, int* val,
                    EmberEventTimeStatistic* stat = NULL ) :
            EmberShmemEvent( api, output, stat ), m_val(val) {}
	~EmberMyPeShmemEvent() {}

    std::string getName() { return "MyPE"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.my_pe( m_val, callback );
    }
private:
    int* m_val;
};

}
}

#endif
