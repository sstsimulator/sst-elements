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

#ifndef _H_EMBER_SHMEM_GENERATOR
#define _H_EMBER_SHMEM_GENERATOR

#include "embergen.h"

#include <sst/elements/hermes/shmemapi.h>

#include "emberShmemInitEv.h"
#include "emberShmemFiniEv.h"
#include "emberShmemMyPeEv.h"
#include "emberShmemNPesEv.h"
#include "emberShmemBarrierEv.h"
#include "emberShmemFenceEv.h"
#include "emberShmemMallocEv.h"
#include "emberShmemFreeEv.h"
#include "emberShmemPutEv.h"
#include "emberShmemPutVEv.h"
#include "emberShmemGetEv.h"
#include "emberShmemGetVEv.h"

using namespace Hermes;

namespace SST {
namespace Ember {

class EmberShmemGenerator : public EmberGenerator {

public:

    typedef std::queue<EmberEvent*> Queue;

	EmberShmemGenerator( Component* owner, Params& params, std::string name );
	~EmberShmemGenerator();
    virtual void completed( const SST::Output*, uint64_t time );

protected:
    inline void enQ_init( Queue& );
    inline void enQ_fini( Queue& );

    inline void enQ_my_pe( Queue&, int* );
    inline void enQ_n_pes( Queue&, int *);
    inline void enQ_barrier( Queue& );
    inline void enQ_fence( Queue& );
    inline void enQ_malloc( Queue&, Hermes::MemAddr*, size_t );
    inline void enQ_free( Queue&, Hermes::MemAddr );

    template <class TYPE>
    inline void enQ_putv( Queue&, Hermes::MemAddr, TYPE, int pe );
    template <class TYPE>
    inline void enQ_getv( Queue&, TYPE*, Hermes::MemAddr, int pe );

    inline void enQ_get( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe );
    inline void enQ_put( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe );

private:
};

static inline Hermes::Shmem::Interface* shmem_cast( Hermes::Interface *in )
{
    return static_cast<Hermes::Shmem::Interface*>(in);
}

void EmberShmemGenerator::enQ_init( Queue& q )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberInitShmemEvent( *shmem_cast(m_api), &getOutput() ) );
}

void EmberShmemGenerator::enQ_fini( Queue& q )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberFiniShmemEvent( *shmem_cast(m_api), &getOutput() ) );
}

void EmberShmemGenerator::enQ_my_pe( Queue& q, int* val )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberMyPeShmemEvent( *shmem_cast(m_api), &getOutput(), val ) );
}
void EmberShmemGenerator::enQ_n_pes( Queue& q, int* val )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberNPesShmemEvent( *shmem_cast(m_api), &getOutput(), val ) );
}

void EmberShmemGenerator::enQ_barrier( Queue& q )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberBarrierShmemEvent( *shmem_cast(m_api), &getOutput() ) );
}

void EmberShmemGenerator::enQ_fence( Queue& q )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberFenceShmemEvent( *shmem_cast(m_api), &getOutput() ) );
}


void EmberShmemGenerator::enQ_malloc( Queue& q, Hermes::MemAddr* ptr, size_t num )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberMallocShmemEvent( *shmem_cast(m_api), &getOutput(), ptr, num ) );
}

void EmberShmemGenerator::enQ_free( Queue& q, Hermes::MemAddr addr )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberFreeShmemEvent( *shmem_cast(m_api), &getOutput(), addr ) );
}

void EmberShmemGenerator::enQ_get( Queue& q, Hermes::MemAddr dest, 
        Hermes::MemAddr src, size_t length, int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberGetShmemEvent( *shmem_cast(m_api), &getOutput(),  
                dest, src, length, pe ) );
}

void EmberShmemGenerator::enQ_put( Queue& q, Hermes::MemAddr dest, 
        Hermes::MemAddr src, size_t length, int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberPutShmemEvent( *shmem_cast(m_api), &getOutput(),  
                dest, src, length, pe ) );
}

template <class TYPE>
void EmberShmemGenerator::enQ_getv( Queue& q, TYPE* laddr, Hermes::MemAddr addr, 
       int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberGetVShmemEvent<TYPE>( *shmem_cast(m_api), &getOutput(),  
                laddr, addr, pe ) );
}
template <class TYPE>
void EmberShmemGenerator::enQ_putv( Queue& q, Hermes::MemAddr addr, 
        TYPE value, int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberPutVShmemEvent<TYPE>( *shmem_cast(m_api), &getOutput(),  
                addr, value, pe ) );
}

}
}

#endif
