// Copyright 2009-2018 NTESS. Under the terms

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

#ifndef _H_EMBER_SHMEM_GENERATOR
#define _H_EMBER_SHMEM_GENERATOR

#include "embergen.h"

#include <sst/core/elementinfo.h>
#include <sst/elements/hermes/shmemapi.h>

#include "embergettimeev.h"
#include "emberShmemInitEv.h"
#include "emberShmemFiniEv.h"
#include "emberShmemMyPeEv.h"
#include "emberShmemNPesEv.h"
#include "emberShmemBarrierEv.h"
#include "emberShmemBarrierAllEv.h"
#include "emberShmemBroadcastEv.h"
#include "emberShmemCollectEv.h"
#include "emberShmemFcollectEv.h"
#include "emberShmemAlltoallEv.h"
#include "emberShmemAlltoallsEv.h"
#include "emberShmemReductionEv.h"
#include "emberShmemFenceEv.h"
#include "emberShmemQuietEv.h"
#include "emberShmemMallocEv.h"
#include "emberShmemFreeEv.h"
#include "emberShmemPutEv.h"
#include "emberShmemPutVEv.h"
#include "emberShmemGetEv.h"
#include "emberShmemGetVEv.h"
#include "emberShmemWaitEv.h"
#include "emberShmemCswapEv.h"
#include "emberShmemSwapEv.h"
#include "emberShmemFaddEv.h"
#include "emberShmemAddEv.h"

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
	inline void enQ_getTime( Queue&, uint64_t* time );

    inline void enQ_init( Queue& );
    inline void enQ_fini( Queue& );

    inline void enQ_my_pe( Queue&, int* );
    inline void enQ_n_pes( Queue&, int *);
    inline void enQ_barrier_all( Queue& );
    inline void enQ_barrier( Queue&, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );
    inline void enQ_broadcast32( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelmes, 
            int PE_root, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );
    inline void enQ_broadcast64( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelmes, 
            int PE_root, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );

    inline void enQ_fcollect32( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelmes, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );
    inline void enQ_fcollect64( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelmes, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );

    inline void enQ_collect32( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelmes, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );
    inline void enQ_collect64( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelmes, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );

    inline void enQ_alltoall32( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelmes, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );
    inline void enQ_alltoall64( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelmes, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );

    inline void enQ_alltoalls32( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, int dst,
            int sst, size_t nelmes, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );
    inline void enQ_alltoalls64( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, int dst,
            int sst, size_t nelmes, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr );


	typedef int Fam_Region_Descriptor;
    inline void enQ_fam_initialize( Queue&, std::string groupName );
    inline void enQ_fam_finalize( Queue&, std::string groupName );
    inline void enQ_fam_create_region( Queue&, std::string groupName, uint64_t size, Fam_Region_Descriptor& rd );
    inline void enQ_fam_create_region( Queue&, Fam_Region_Descriptor rd );
	inline void enQ_fam_get_nonblocking( Queue&, Hermes::MemAddr, Fam_Region_Descriptor rd, uint64_t offset, uint64_t nbytes );

#define declareOp( type, op) \
    inline void enQ_##type##_##op##_to_all( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, int nelmes, \
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync );\

#define declareMathOps( op )\
    declareOp( short, op )\
    declareOp( int, op )\
    declareOp( long, op )\
    declareOp( longlong, op )\
    declareOp( float, op )\
    declareOp( double, op )\
    declareOp( longdouble, op )\

#define declareBitOps( op )\
    declareOp( short, op )\
    declareOp( int, op )\
    declareOp( long, op )\
    declareOp( longlong, op )\

    declareBitOps( and )
    declareBitOps( or )
    declareBitOps( xor )

    declareMathOps( min )
    declareMathOps( max )
    declareMathOps( sum )
    declareMathOps( prod )

    inline void enQ_fence( Queue& );
    inline void enQ_quiet( Queue& );
    inline void enQ_malloc( Queue&, Hermes::MemAddr*, size_t, bool backed = true );
    inline void enQ_free( Queue&, Hermes::MemAddr );

    template <class TYPE>
    inline void enQ_putv( Queue&, Hermes::MemAddr, TYPE, int pe );
    template <class TYPE>
    inline void enQ_getv( Queue&, TYPE*, Hermes::MemAddr, int pe );

    template <class TYPE>
    inline void enQ_wait( Queue&, Hermes::MemAddr, TYPE );
    template <class TYPE>
    inline void enQ_wait_until( Queue&, Hermes::MemAddr, Hermes::Shmem::WaitOp, TYPE );

    inline void enQ_get( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe );
    inline void enQ_put( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe );
    inline void enQ_get_nbi( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe );
    inline void enQ_put_nbi( Queue&, Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe );

    template <class TYPE>
    inline void enQ_add( Queue&, Hermes::MemAddr, TYPE*, int pe );
    template <class TYPE>
    inline void enQ_fadd( Queue&, TYPE*, Hermes::MemAddr, TYPE*, int pe );


    template <class TYPE>
    inline void enQ_swap( Queue&, TYPE*, Hermes::MemAddr, TYPE* value, int pe );
    template <class TYPE>
    inline void enQ_cswap( Queue&, TYPE*, Hermes::MemAddr, TYPE* cond, TYPE* value, int pe );

private:
};

void EmberShmemGenerator::enQ_fam_initialize( Queue& q, std::string groupName ) {
    verbose(CALL_INFO,2,0,"\n");
	enQ_init(q);
}

void EmberShmemGenerator::enQ_fam_finalize( Queue&, std::string groupName ) {
    verbose(CALL_INFO,2,0,"\n");
}

void EmberShmemGenerator::enQ_fam_create_region( Queue&, std::string groupName, uint64_t size, Fam_Region_Descriptor& rd ) {
    verbose(CALL_INFO,2,0,"\n");
}

void EmberShmemGenerator::enQ_fam_create_region( Queue&, Fam_Region_Descriptor rd ) {
    verbose(CALL_INFO,2,0,"\n");
}

void EmberShmemGenerator::enQ_fam_get_nonblocking( Queue& q, Hermes::MemAddr dest, Fam_Region_Descriptor rd, uint64_t offset, uint64_t nbytes ){
    verbose(CALL_INFO,2,0,"\n");

	assert( m_famAddrMapper );
	uint64_t localOffset;
	int 	 node;
	m_famAddrMapper->getAddr( getOutput(), offset, node, localOffset );

    verbose(CALL_INFO,2,0,"node=%" PRIx32 " localOffset=0x%" PRIx64 "\n", node, localOffset );
    verbose(CALL_INFO,2,0,"0x%" PRIx64" %p\n", dest.getSimVAddr(), dest.getBacking() );

   	Hermes::MemAddr src( localOffset, NULL );
	enQ_get_nbi( q, dest, src, nbytes , node );
}

static inline Hermes::Shmem::Interface* shmem_cast( Hermes::Interface *in )
{
    return static_cast<Hermes::Shmem::Interface*>(in);
}

void EmberShmemGenerator::enQ_getTime( Queue& q, uint64_t* time )
{
    q.push( new EmberGetTimeEvent( &getOutput(), time ) );
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

void EmberShmemGenerator::enQ_barrier_all( Queue& q )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberBarrierAllShmemEvent( *shmem_cast(m_api), &getOutput() ) );
}

void EmberShmemGenerator::enQ_barrier( Queue& q, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberBarrierShmemEvent( *shmem_cast(m_api), &getOutput(), PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

void EmberShmemGenerator::enQ_broadcast32( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems, 
            int PE_root, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberBroadcastShmemEvent( *shmem_cast(m_api), &getOutput(), 
                    dest.getSimVAddr(), src.getSimVAddr(), nelems * 4, PE_root, PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

void EmberShmemGenerator::enQ_broadcast64( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems, 
            int PE_root, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberBroadcastShmemEvent( *shmem_cast(m_api), &getOutput(), 
                    dest.getSimVAddr(), src.getSimVAddr(), nelems * 8, PE_root, PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

void EmberShmemGenerator::enQ_fcollect32( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberFcollectShmemEvent( *shmem_cast(m_api), &getOutput(), 
                    dest.getSimVAddr(), src.getSimVAddr(), nelems * 4, PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

void EmberShmemGenerator::enQ_fcollect64( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberFcollectShmemEvent( *shmem_cast(m_api), &getOutput(), 
                    dest.getSimVAddr(), src.getSimVAddr(), nelems * 8, PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

void EmberShmemGenerator::enQ_collect32( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberCollectShmemEvent( *shmem_cast(m_api), &getOutput(), 
                    dest.getSimVAddr(), src.getSimVAddr(), nelems * 4, PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

void EmberShmemGenerator::enQ_collect64( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberCollectShmemEvent( *shmem_cast(m_api), &getOutput(), 
                    dest.getSimVAddr(), src.getSimVAddr(), nelems * 8, PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

void EmberShmemGenerator::enQ_alltoall32( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberAlltoallShmemEvent( *shmem_cast(m_api), &getOutput(), 
                    dest.getSimVAddr(), src.getSimVAddr(), nelems * 4, PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

void EmberShmemGenerator::enQ_alltoall64( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems, 
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberAlltoallShmemEvent( *shmem_cast(m_api), &getOutput(), 
                    dest.getSimVAddr(), src.getSimVAddr(), nelems * 8, PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

void EmberShmemGenerator::enQ_alltoalls32( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, 
        int dst, int sst, size_t nelems, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberAlltoallsShmemEvent( *shmem_cast(m_api), &getOutput(), 
                    dest.getSimVAddr(), src.getSimVAddr(), dst, sst, nelems, 4, PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

void EmberShmemGenerator::enQ_alltoalls64( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, 
        int dst, int sst, size_t nelems, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberAlltoallsShmemEvent( *shmem_cast(m_api), &getOutput(), 
                    dest.getSimVAddr(), src.getSimVAddr(), dst, sst, nelems, 8, PE_start,
                        logPE_stride, PE_size, pSync.getSimVAddr() ) );
}

#define defineReduce( type1, type2, op1, op2 ) \
void EmberShmemGenerator::enQ_##type1##_##op1##_to_all( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, int nelems, \
            int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )\
{\
    verbose(CALL_INFO,2,0,"\n");\
    q.push( new EmberReductionShmemEvent( *shmem_cast(m_api), &getOutput(), \
                    dest.getSimVAddr(), src.getSimVAddr(), nelems, PE_start, logPE_stride, \
                    PE_size, pSync.getSimVAddr(), Hermes::Shmem::op2, Hermes::Value::type2 ) ); \
}\

#define defineBitOp( op1, op2 ) \
    defineReduce(short,Short,op1,op2) \
    defineReduce(int,Int,op1,op2) \
    defineReduce(long,Long,op1,op2) \
    defineReduce(longlong,LongLong,op1,op2)

#define defineMathOp( op1, op2 ) \
    defineReduce(float,Float,op1,op2) \
    defineReduce(double,Double,op1,op2) \
    defineReduce(longdouble,LongDouble,op1,op2) \
    defineReduce(short,Short,op1,op2) \
    defineReduce(int,Int,op1,op2) \
    defineReduce(long,Long,op1,op2) \
    defineReduce(longlong,LongLong,op1,op2)

defineBitOp(or,OR)
defineBitOp(xor,XOR)
defineBitOp(and,AND)

defineMathOp(max,MAX)
defineMathOp(min,MIN)
defineMathOp(sum,SUM)
defineMathOp(prod,PROD)

void EmberShmemGenerator::enQ_fence( Queue& q )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberFenceShmemEvent( *shmem_cast(m_api), &getOutput() ) );
}

void EmberShmemGenerator::enQ_quiet( Queue& q )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberQuietShmemEvent( *shmem_cast(m_api), &getOutput() ) );
}


void EmberShmemGenerator::enQ_malloc( Queue& q, Hermes::MemAddr* ptr, size_t num, bool backed )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberMallocShmemEvent( *shmem_cast(m_api), &getOutput(), ptr, num, backed ) );
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
                dest.getSimVAddr(), src.getSimVAddr(), length, pe, true ) );
}

void EmberShmemGenerator::enQ_get_nbi( Queue& q, Hermes::MemAddr dest, 
        Hermes::MemAddr src, size_t length, int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberGetShmemEvent( *shmem_cast(m_api), &getOutput(),  
                dest.getSimVAddr(), src.getSimVAddr(), length, pe, false ) );
}

template <class TYPE>
void EmberShmemGenerator::enQ_getv( Queue& q, TYPE* laddr, Hermes::MemAddr addr, 
       int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberGetVShmemEvent( *shmem_cast(m_api), &getOutput(),  
                Hermes::Value(laddr), addr.getSimVAddr(), pe ) );
}

void EmberShmemGenerator::enQ_put( Queue& q, Hermes::MemAddr dest, 
        Hermes::MemAddr src, size_t length, int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberPutShmemEvent( *shmem_cast(m_api), &getOutput(),  
                dest.getSimVAddr(), src.getSimVAddr(), length, pe, true ) );
}

void EmberShmemGenerator::enQ_put_nbi( Queue& q, Hermes::MemAddr dest, 
        Hermes::MemAddr src, size_t length, int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberPutShmemEvent( *shmem_cast(m_api), &getOutput(),  
                dest.getSimVAddr(), src.getSimVAddr(), length, pe, false ) );
}

template <class TYPE>
void EmberShmemGenerator::enQ_putv( Queue& q, Hermes::MemAddr addr, 
        TYPE value, int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberPutvShmemEvent( *shmem_cast(m_api), &getOutput(),  
                addr.getSimVAddr(), Hermes::Value( (TYPE) value ), pe ) );
}

template <class TYPE>
void EmberShmemGenerator::enQ_wait( Queue& q, Hermes::MemAddr addr, TYPE value )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberWaitShmemEvent( *shmem_cast(m_api), &getOutput(),  
                addr.getSimVAddr(), Hermes::Shmem::NE, Hermes::Value( (TYPE) value ) ) );
}

template <class TYPE>
void EmberShmemGenerator::enQ_wait_until( Queue& q, Hermes::MemAddr addr, Hermes::Shmem::WaitOp op, TYPE value )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberWaitShmemEvent( *shmem_cast(m_api), &getOutput(),  
                addr.getSimVAddr(), op, Hermes::Value( (TYPE) value ) ) );
}

template <class TYPE>
void EmberShmemGenerator::enQ_add( Queue& q, Hermes::MemAddr addr, TYPE* value,  
       int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberAddShmemEvent( *shmem_cast(m_api), &getOutput(),  
                addr.getSimVAddr(), Hermes::Value(value), pe ) );
}

template <class TYPE>
void EmberShmemGenerator::enQ_fadd( Queue& q, TYPE* result, Hermes::MemAddr addr, TYPE* value,  
       int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberFaddShmemEvent( *shmem_cast(m_api), &getOutput(),  
                Hermes::Value(result), addr.getSimVAddr(), Hermes::Value(value), pe ) );
}

template <class TYPE>
void EmberShmemGenerator::enQ_swap( Queue& q, TYPE* result, Hermes::MemAddr addr, TYPE* value,  
       int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberSwapShmemEvent( *shmem_cast(m_api), &getOutput(),  
                Hermes::Value(result), addr.getSimVAddr(), Hermes::Value(value), pe ) );
}

template <class TYPE>
void EmberShmemGenerator::enQ_cswap( Queue& q, TYPE* result, Hermes::MemAddr addr, TYPE* cond, TYPE* value,  
       int pe )
{
    verbose(CALL_INFO,2,0,"\n");
    q.push( new EmberCswapShmemEvent( *shmem_cast(m_api), &getOutput(),  
                Hermes::Value(result), addr.getSimVAddr(), Hermes::Value(cond), Hermes::Value(value), pe ) );
}


}
}

#endif
