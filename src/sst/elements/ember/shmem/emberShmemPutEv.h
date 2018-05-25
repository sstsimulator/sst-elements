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


#ifndef _H_EMBER_SHMEM_PUT_EVENT
#define _H_EMBER_SHMEM_PUT_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

class EmberPutShmemEvent : public EmberShmemEvent {

public:
	EmberPutShmemEvent( Shmem::Interface& api, Output* output,
            Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, int pe, bool blocking, 
            EmberEventTimeStatistic* stat = NULL ) :
            EmberShmemEvent( api, output, stat ), 
            m_dest(dest), m_src(src), m_length(length), m_pe(pe), m_blocking(blocking) {}
	~EmberPutShmemEvent() {}

    std::string getName() { return "Malloc"; }

    void issue( uint64_t time, Callback callback ) {

        EmberEvent::issue( time );
        if ( m_blocking ) {
        	m_api.put( m_dest, m_src, m_length, m_pe, callback );
        }else{
            m_api.put_nbi( m_dest, m_src, m_length, m_pe, callback );
        }
    }
private:
    Hermes::Vaddr m_dest;
    Hermes::Vaddr m_src;
    size_t m_length;
    int m_pe;
    bool m_blocking;
};

}
}

#endif
