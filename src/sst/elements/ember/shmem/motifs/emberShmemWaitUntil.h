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


#ifndef _H_EMBER_SHMEM_WAITUNTIL
#define _H_EMBER_SHMEM_WAITUNTIL

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemWaitUntilGenerator : public EmberShmemGenerator {

public:
	EmberShmemWaitUntilGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemWaitUntil" ), m_phase(0) 
	{ 
         m_opName = params.find<std::string>("arg.op", "NE");
         printf("%s\n",m_opName.c_str()); 

         m_initValue = 0; 
         m_waitValue = m_initValue;
         if ( 0 == m_opName.compare("LTE") ) {
            m_waitValue = -1;
            m_op = Hermes::Shmem::LTE;
            m_putValue = -1;
         } else if ( 0 == m_opName.compare("LT") ) {
            m_op = Hermes::Shmem::LT;
            m_putValue = -1;
         } else if ( 0 == m_opName.compare("E") ) {
            m_op = Hermes::Shmem::E;
            m_waitValue = 1;
            m_putValue = 1;
         } else if ( 0 == m_opName.compare("NE") ) {
            m_op = Hermes::Shmem::NE;
            m_putValue = 1;
         } else if ( 0 == m_opName.compare("GT") ) {
            m_op = Hermes::Shmem::GT;
            m_putValue = 1;
         } else if ( 0 == m_opName.compare("GTE") ) {
            m_op = Hermes::Shmem::GTE;
            m_waitValue = 1;
            m_putValue = 1;
         } else {
             assert(0);
         }
    }

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
            m_addr.at<int>(0) = m_initValue;
            enQ_barrier( evQ );
            
            if ( m_my_pe == 0 ) {
                enQ_wait_until( evQ, m_addr, m_op, m_waitValue );
            } else {
                enQ_putv( evQ, m_addr, m_putValue, 0 );
            }

            break;

        case 4:
            if ( m_my_pe == 0 ) {
                printf("%d:%s: %s init=%#x wait=%#x put=%#x got=%#x\n",m_my_pe, getMotifName().c_str(),
                        m_opName.c_str(), m_initValue, m_waitValue, m_putValue, m_addr.at<int>(0));
            }
		    ret = true;
            break;
        }
        ++m_phase;
        return ret;
	}
  private:
    Hermes::Shmem::WaitOp m_op;
    Hermes::MemAddr       m_addr;
    std::string m_opName;
    int m_initValue;
    int m_putValue;
    int m_waitValue;
    int m_phase;
    int m_my_pe;
    int m_n_pes;
};

}
}

#endif
