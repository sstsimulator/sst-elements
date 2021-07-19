// Copyright 2009-2021 NTESS. Under the terms

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

#ifndef _H_EMBER_SHMEM_LIB
#define _H_EMBER_SHMEM_LIB

#include "libs/emberLib.h"
#include "sst/elements/hermes/shmemapi.h"

#include "shmem/emberShmemInitEv.h"
#include "shmem/emberShmemFiniEv.h"
#include "shmem/emberShmemMyPeEv.h"
#include "shmem/emberShmemNPesEv.h"
#include "shmem/emberShmemBarrierEv.h"
#include "shmem/emberShmemBarrierAllEv.h"
#include "shmem/emberShmemBroadcastEv.h"
#include "shmem/emberShmemCollectEv.h"
#include "shmem/emberShmemFcollectEv.h"
#include "shmem/emberShmemAlltoallEv.h"
#include "shmem/emberShmemAlltoallsEv.h"
#include "shmem/emberShmemReductionEv.h"
#include "shmem/emberShmemFenceEv.h"
#include "shmem/emberShmemQuietEv.h"
#include "shmem/emberShmemMallocEv.h"
#include "shmem/emberShmemFreeEv.h"
#include "shmem/emberShmemPutEv.h"
#include "shmem/emberShmemPutVEv.h"
#include "shmem/emberShmemGetEv.h"
#include "shmem/emberShmemGetVEv.h"
#include "shmem/emberShmemWaitEv.h"
#include "shmem/emberShmemCswapEv.h"
#include "shmem/emberShmemSwapEv.h"
#include "shmem/emberShmemFaddEv.h"
#include "shmem/emberShmemAddEv.h"

#include "shmem/emberFamGet_Ev.h"
#include "shmem/emberFamPut_Ev.h"
#include "shmem/emberFamScatterv_Ev.h"
#include "shmem/emberFamScatter_Ev.h"
#include "shmem/emberFamGatherv_Ev.h"
#include "shmem/emberFamGather_Ev.h"
#include "shmem/emberFamAddEv.h"
#include "shmem/emberFamCswapEv.h"

using namespace Hermes;

namespace SST {
namespace Ember {

class EmberShmemLib : public EmberLib {

  public:

    SST_ELI_REGISTER_MODULE(
        EmberShmemLib,
        "ember",
        "shmemLib",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        "SST::Ember::EmberShmemLib"
    )

    SST_ELI_DOCUMENT_PARAMS(
	)

    typedef std::queue<EmberEvent*> Queue;

	EmberShmemLib( Params& params ) {}

	void fam_initialize( Queue& q, std::string groupName ) {
		init(q);
	}

	template <class TYPE>
	void fam_add( Queue& q, Shmem::Fam_Descriptor fd, uint64_t offset, TYPE* value )
	{
		q.push( new EmberFamAddEvent( api(), m_output, fd, offset, Hermes::Value(value) ) );
	}
	template <class TYPE>
	void fam_compare_swap( Queue& q, TYPE* result, Shmem::Fam_Descriptor fd, uint64_t offset, TYPE* oldValue, TYPE* newValue )
	{
		q.push( new EmberFamCswapEvent( api(), m_output, Hermes::Value(result), fd, offset, Hermes::Value(oldValue), Hermes::Value(newValue) ) );
	}

	void fam_get_nonblocking( Queue& q, Hermes::MemAddr dest, Shmem::Fam_Descriptor fd,
		uint64_t offset, uint64_t nbytes )
	{
		q.push( new EmberFamGet_Event( api(), m_output, dest.getSimVAddr(), fd, offset, nbytes, false ) );
	}

	void fam_get_blocking( Queue& q, Hermes::MemAddr dest, Shmem::Fam_Descriptor fd,
		uint64_t offset, uint64_t nbytes )
	{
		q.push( new EmberFamGet_Event( api(), m_output, dest.getSimVAddr(), fd, offset, nbytes, true ) );
	}

	void fam_put_nonblocking( Queue& q, Shmem::Fam_Descriptor fd, uint64_t offset,
		Hermes::MemAddr src, uint64_t nbytes )
	{
		q.push( new EmberFamPut_Event( api(), m_output, fd, offset, src.getSimVAddr(), nbytes, false ) );
	}

	void fam_put_blocking( Queue& q, Shmem::Fam_Descriptor fd, uint64_t offset,
		Hermes::MemAddr src, uint64_t nbytes )
	{
		q.push( new EmberFamPut_Event( api(), m_output, fd, offset, src.getSimVAddr(), nbytes, true ) );
	}

	void fam_scatterv_blocking( Queue& q, Hermes::MemAddr src, Shmem::Fam_Descriptor fd,
		uint64_t nblocks, std::vector<uint64_t> indexes, uint64_t blockSize )
	{
		q.push( new EmberFamScatterv_Event( api(), m_output, src.getSimVAddr(), fd, nblocks, indexes, blockSize, true ) );
	}

	void fam_scatter_blocking( Queue& q, Hermes::MemAddr src, Shmem::Fam_Descriptor fd,
		uint64_t nblocks, uint64_t firstBlock, uint64_t stride, uint64_t blockSize )
	{
		q.push( new EmberFamScatter_Event( api(), m_output, src.getSimVAddr(), fd, nblocks, firstBlock, stride, blockSize, true ) );
	}

	void fam_scatterv_nonblocking( Queue& q, Hermes::MemAddr src, Shmem::Fam_Descriptor fd,
		uint64_t nblocks, std::vector<uint64_t> indexes, uint64_t blockSize )
	{
		q.push( new EmberFamScatterv_Event( api(), m_output, src.getSimVAddr(), fd, nblocks, indexes, blockSize, false ) );
	}

	void fam_scatter_nonblocking( Queue& q, Hermes::MemAddr src, Shmem::Fam_Descriptor fd,
		uint64_t nblocks, uint64_t firstBlock, uint64_t stride, uint64_t blockSize )
	{
		q.push( new EmberFamScatter_Event( api(), m_output, src.getSimVAddr(), fd, nblocks, firstBlock, stride, blockSize, false ) );
	}

	void fam_gatherv_blocking( Queue& q, Hermes::MemAddr dest, Shmem::Fam_Descriptor fd,
		uint64_t nblocks, std::vector<uint64_t> indexes, uint64_t blockSize )
	{
		q.push( new EmberFamGatherv_Event( api(), m_output, dest.getSimVAddr(), fd, nblocks, indexes, blockSize, true ) );
	}
	void fam_gather_blocking( Queue& q, Hermes::MemAddr dest, Shmem::Fam_Descriptor fd,
		uint64_t nblocks, uint64_t firstBlock, uint64_t stride, uint64_t blockSize )
	{
		q.push( new EmberFamGather_Event( api(), m_output, dest.getSimVAddr(), fd, nblocks, firstBlock, stride, blockSize, true ) );
	}

	void fam_gatherv_nonblocking( Queue& q, Hermes::MemAddr dest, Shmem::Fam_Descriptor fd,
		uint64_t nblocks, std::vector<uint64_t> indexes, uint64_t blockSize )
	{
		q.push( new EmberFamGatherv_Event( api(), m_output, dest.getSimVAddr(), fd, nblocks, indexes, blockSize, false ) );
	}
	void fam_gather_nonblocking( Queue& q, Hermes::MemAddr dest, Shmem::Fam_Descriptor fd,
		uint64_t nblocks, uint64_t firstBlock, uint64_t stride, uint64_t blockSize )
	{
		q.push( new EmberFamGather_Event( api(), m_output, dest.getSimVAddr(), fd, nblocks, firstBlock, stride, blockSize, false ) );
	}

	void getTime( Queue& q, uint64_t* time )
	{
		q.push( new EmberGetTimeEvent( m_output, time ) );
	}

	void init( Queue& q )
	{
		q.push( new EmberInitShmemEvent( api(), m_output ) );
	}

	void fini( Queue& q ) {
		q.push( new EmberFiniShmemEvent( api(), m_output ) );
	}

	void my_pe( Queue& q, int* val ) {
		q.push( new EmberMyPeShmemEvent( api(), m_output, val ) );
	}

	void n_pes( Queue& q, int* val ) {
		q.push( new EmberNPesShmemEvent( api(), m_output, val ) );
	}

	void barrier_all( Queue& q ) {
		q.push( new EmberBarrierAllShmemEvent( api(), m_output ) );
	}

	void barrier( Queue& q, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync ) {
		q.push( new EmberBarrierShmemEvent( api(), m_output, PE_start, logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

	void broadcast32( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems,
				int PE_root, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
	{
		q.push( new EmberBroadcastShmemEvent( api(), m_output,
						dest.getSimVAddr(), src.getSimVAddr(), nelems * 4, PE_root, PE_start,
							logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

	void broadcast64( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems,
				int PE_root, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
	{
		q.push( new EmberBroadcastShmemEvent( api(), m_output,
						dest.getSimVAddr(), src.getSimVAddr(), nelems * 8, PE_root, PE_start,
							logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

	void fcollect32( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems,
				int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
	{
		q.push( new EmberFcollectShmemEvent( api(), m_output,
						dest.getSimVAddr(), src.getSimVAddr(), nelems * 4, PE_start,
							logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

	void fcollect64( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems,
				int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
	{
		q.push( new EmberFcollectShmemEvent( api(), m_output,
						dest.getSimVAddr(), src.getSimVAddr(), nelems * 8, PE_start,
							logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

	void collect32( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems,
				int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
	{
		q.push( new EmberCollectShmemEvent( api(), m_output,
						dest.getSimVAddr(), src.getSimVAddr(), nelems * 4, PE_start,
							logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

	void collect64( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems,
				int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
	{
		q.push( new EmberCollectShmemEvent( api(), m_output,
						dest.getSimVAddr(), src.getSimVAddr(), nelems * 8, PE_start,
							logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

	void alltoall32( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems,
				int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
	{
		q.push( new EmberAlltoallShmemEvent( api(), m_output,
						dest.getSimVAddr(), src.getSimVAddr(), nelems * 4, PE_start,
							logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

	void alltoall64( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t nelems,
				int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
	{
		q.push( new EmberAlltoallShmemEvent( api(), m_output,
						dest.getSimVAddr(), src.getSimVAddr(), nelems * 8, PE_start,
							logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

	void alltoalls32( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src,
			int dst, int sst, size_t nelems, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
	{
		q.push( new EmberAlltoallsShmemEvent( api(), m_output,
						dest.getSimVAddr(), src.getSimVAddr(), dst, sst, nelems, 4, PE_start,
							logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

	void alltoalls64( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src,
			int dst, int sst, size_t nelems, int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )
	{
		q.push( new EmberAlltoallsShmemEvent( api(), m_output,
						dest.getSimVAddr(), src.getSimVAddr(), dst, sst, nelems, 8, PE_start,
							logPE_stride, PE_size, pSync.getSimVAddr() ) );
	}

#define defineReduce( type1, type2, op1, op2 ) \
	void type1##_##op1##_to_all( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, int nelems, \
				int PE_start, int logPE_stride, int PE_size, Hermes::MemAddr pSync )\
	{\
		q.push( new EmberReductionShmemEvent( api(), m_output, \
						dest.getSimVAddr(), src.getSimVAddr(), nelems, PE_start, logPE_stride, \
						PE_size, pSync.getSimVAddr(), Hermes::Shmem::op2, Hermes::Value::type2 ) ); \
	}

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

	void fence( Queue& q ) {
		q.push( new EmberFenceShmemEvent( api(), m_output ) );
	}

	void quiet( Queue& q ) {
		q.push( new EmberQuietShmemEvent( api(), m_output ) );
	}

	void malloc( Queue& q, Hermes::MemAddr* ptr, size_t num, bool backed = true ) {
		q.push( new EmberMallocShmemEvent( api(), m_output, ptr, num, backed ) );
	}

	void free( Queue& q, Hermes::MemAddr addr ) {
		q.push( new EmberFreeShmemEvent( api(), m_output, addr ) );
	}

	void get( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe ) {
		q.push( new EmberGetShmemEvent( api(), m_output,  dest.getSimVAddr(), src.getSimVAddr(), length, pe, true ) );
	}

	void get_nbi( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe ) {
		q.push( new EmberGetShmemEvent( api(), m_output,  dest.getSimVAddr(), src.getSimVAddr(), length, pe, false ) );
	}

	template <class TYPE>
	void getv( Queue& q, TYPE* laddr, Hermes::MemAddr addr, int pe ) {
		q.push( new EmberGetVShmemEvent( api(), m_output,  Hermes::Value(laddr), addr.getSimVAddr(), pe ) );
	}

	void put( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe ) {
		q.push( new EmberPutShmemEvent( api(), m_output,  dest.getSimVAddr(), src.getSimVAddr(), length, pe, true ) );
	}

	void put_nbi( Queue& q, Hermes::MemAddr dest, Hermes::MemAddr src, size_t length, int pe ) {
		q.push( new EmberPutShmemEvent( api(), m_output,  dest.getSimVAddr(), src.getSimVAddr(), length, pe, false ) );
	}

	template <class TYPE>
	void putv( Queue& q, Hermes::MemAddr addr, TYPE value, int pe ) {
		q.push( new EmberPutvShmemEvent( api(), m_output,  addr.getSimVAddr(), Hermes::Value( (TYPE) value ), pe ) );
	}

	template <class TYPE>
	void wait( Queue& q, Hermes::MemAddr addr, TYPE value ) {
		q.push( new EmberWaitShmemEvent( api(), m_output,  addr.getSimVAddr(), Hermes::Shmem::NE, Hermes::Value( (TYPE) value ) ) );
	}

	template <class TYPE>
	void wait_until( Queue& q, Hermes::MemAddr addr, Hermes::Shmem::WaitOp op, TYPE value ) {
		q.push( new EmberWaitShmemEvent( api(), m_output,  addr.getSimVAddr(), op, Hermes::Value( (TYPE) value ) ) );
	}

	template <class TYPE>
	void add( Queue& q, Hermes::MemAddr addr, TYPE* value,  int pe ) {
		q.push( new EmberAddShmemEvent( api(), m_output,
					addr.getSimVAddr(), Hermes::Value(value), pe ) );
	}

	template <class TYPE>
	void fadd( Queue& q, TYPE* result, Hermes::MemAddr addr, TYPE* value,  int pe ) {
		q.push( new EmberFaddShmemEvent( api(), m_output,
					Hermes::Value(result), addr.getSimVAddr(), Hermes::Value(value), pe ) );
	}

	template <class TYPE>
	void swap( Queue& q, TYPE* result, Hermes::MemAddr addr, TYPE* value,  int pe ) {
		q.push( new EmberSwapShmemEvent( api(), m_output,
					Hermes::Value(result), addr.getSimVAddr(), Hermes::Value(value), pe ) );
	}

	template <class TYPE>
	void cswap( Queue& q, TYPE* result, Hermes::MemAddr addr, TYPE* cond, TYPE* value,  int pe ) {
		q.push( new EmberCswapShmemEvent( api(), m_output,
					Hermes::Value(result), addr.getSimVAddr(), Hermes::Value(cond), Hermes::Value(value), pe ) );
	}

  private:
	Shmem::Interface& api() { return *static_cast<Shmem::Interface*>(m_api); }
};

}
}

#endif
