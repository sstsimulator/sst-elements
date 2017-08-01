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


#ifndef _H_EMBER_SHMEM_GETV_EVENT
#define _H_EMBER_SHMEM_GETV_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

template < class TYPE >
class EmberGetVShmemEvent : public EmberShmemEvent {

public:
	EmberGetVShmemEvent( Shmem::Interface& api, Output* output,
            TYPE* dest, Hermes::MemAddr src, int pe, 
            EmberEventTimeStatistic* stat = NULL ) :
            EmberShmemEvent( api, output, stat ), 
            m_dest(dest), m_src(src), m_length(sizeof(TYPE)), m_pe(pe) {}
	~EmberGetVShmemEvent() {}

    std::string getName() { return "Malloc"; }

    void issue( uint64_t time, MP::Functor* functor ) {

        EmberEvent::issue( time );
        m_api.getv( m_dest, m_src, m_length, m_pe, functor );
    }
private:
    TYPE*  m_dest;
    Hermes::MemAddr m_src;
    size_t m_length;
    int m_pe;
};

}
}

#endif
