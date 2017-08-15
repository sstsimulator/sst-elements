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


#ifndef _H_EMBER_SHMEM_SWAP
#define _H_EMBER_SHMEM_SWAP

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemSwapGenerator : public EmberShmemGenerator {

public:
    EmberShmemSwapGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemSwap" ), m_phase(0) 
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
            enQ_malloc( evQ, &m_addr, 4 );
            break;

        case 3:
            
            printf("%d:%s: simVAddr %#" PRIx64 " backing %p\n",m_my_pe, 
                    getMotifName().c_str(), m_addr.getSimVAddr(), m_addr.getBacking() );

            if ( m_my_pe == 0 ) {
                m_addr.at<int>(0) = 10;
            }

            enQ_barrier_all( evQ );

            if ( m_my_pe == 1 ) {
                m_value = 19;
                enQ_swap( evQ, 
                    &m_result,
                    m_addr,
                    &m_value,
                    0 );
            }
            enQ_barrier_all( evQ );
            break;

        case 4:
            if ( 0 == m_my_pe ) {
                printf("%d:%s: Swap result=%#" PRIx32 "\n",m_my_pe, getMotifName().c_str(), m_addr.at<int>(0));
                assert( m_addr.at<int>(0) == 19 );
            } else {
                printf("%d:%s: Swap result=%#" PRIx32 "\n",m_my_pe, getMotifName().c_str(), m_result);
                assert ( m_result == 10 ); 
            }
		    ret = true;
        }
        ++m_phase;
        return ret;
	}
  private:
    Hermes::MemAddr m_addr;
    int m_value;
    int m_result;
    int m_phase;
    int m_my_pe;
    int m_n_pes;
};

}
}

#endif
