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


#ifndef _H_EMBER_SHMEM_WAIT
#define _H_EMBER_SHMEM_WAIT

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemWaitGenerator : public EmberShmemGenerator {

public:
	EmberShmemWaitGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemWait" ), m_phase(0) 
	{ }

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        bool ret = false;
        switch ( m_phase ) {
        case 0:
            enQ_init( evQ );
            break;
        case 1:
            enQ_n_pes( evQ, &m_n_pes );
            enQ_my_pe( evQ, &m_my_pe );
            break;

        case 2:

            printf("%d:%s: %d\n",m_my_pe, getMotifName().c_str(),m_n_pes);
            enQ_malloc( evQ, &m_addr, 1000*2 );
            enQ_barrier( evQ );
            break;

        case 3:

            memset( &m_addr[0], 0, 8 );
            enQ_barrier( evQ );
            
            if ( m_my_pe == 0 ) {
                enQ_wait( evQ, m_addr, (int) 0 );
            } else {
                enQ_putv( evQ, m_addr, (int) (0xdead0000 + m_my_pe), 0 );
            }

            break;

        case 4:
            if ( m_my_pe == 0 ) {
                printf("%d:%s: %#x\n",m_my_pe, getMotifName().c_str(),
                        *((int*)&m_addr[0]));
            }
		    ret = true;
            break;
        }
        ++m_phase;
        return ret;
	}
  private:
    Hermes::MemAddr m_addr;
    int m_local;
    int m_phase;
    int m_my_pe;
    int m_n_pes;
};

}
}

#endif
