// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_EMBER_MPI_GENERATOR
#define _H_EMBER_MPI_GENERATOR

#include "embergen.h"
#include "libs/emberMpiLib.h"

namespace SST {
namespace Ember {

#define enQ_init mpi().init
#define enQ_fini mpi().fini
#define enQ_rank mpi().rank
#define enQ_size mpi().size

#define enQ_makeProgress mpi().makeProgress

#define enQ_send mpi().send
#define enQ_recv mpi().recv
#define enQ_isend mpi().isend
#define enQ_irecv mpi().irecv
#define enQ_cancel mpi().cancel
#define enQ_sendrecv mpi().sendrecv

#define enQ_wait mpi().wait
#define enQ_waitany mpi().waitany
#define enQ_waitall mpi().waitall
#define enQ_test mpi().test
#define enQ_testany mpi().testany

#define enQ_barrier mpi().barrier
#define enQ_bcast mpi().bcast
#define enQ_scatter mpi().scatter
#define enQ_scatterv mpi().scatterv
#define enQ_reduce mpi().reduce
#define enQ_allreduce mpi().allreduce
#define enQ_alltoall mpi().alltoall
#define enQ_alltoallv mpi().alltoallv
#define enQ_allgather mpi().allgather
#define enQ_allgatherv mpi().allgatherv

#define enQ_commSplit mpi().commSplit
#define enQ_commCreate mpi().commCreate
#define enQ_commDestroy mpi().commDestroy

class EmberMessagePassingGenerator : public EmberGenerator {

public:

	SST_ELI_DOCUMENT_PARAMS(
		{"rankmapper","Sets the rank mapper module",""},
	)
	/* PARAMS
		rankmap.*
	*/

	SST_ELI_DOCUMENT_STATISTICS(
        { "time-Init", "Time spent in Init event",          "ns", 1 },
        { "time-Finalize", "Time spent in Finalize event",  "ns", 1 },
        { "time-Rank", "Time spent in Rank event",          "ns", 1 },
        { "time-Size", "Time spent in Size event",          "ns", 1 },
        { "time-Send", "Time spent in Recv event",          "ns", 1 },
        { "time-Recv", "Time spent in Recv event",          "ns", 1 },
        { "time-Irecv", "Time spent in Irecv event",        "ns", 1 },
        { "time-Isend", "Time spent in Isend event",        "ns", 1 },
        { "time-Wait", "Time spent in Wait event",          "ns", 1 },
        { "time-Waitall", "Time spent in Waitall event",    "ns", 1 },
        { "time-Waitany", "Time spent in Waitany event",    "ns", 1 },
        { "time-Compute", "Time spent in Compute event",    "ns", 1 },
        { "time-Barrier", "Time spent in Barrier event",    "ns", 1 },
        { "time-Alltoallv", "Time spent in Alltoallv event", "ns", 1 },
        { "time-Alltoall", "Time spent in Alltoall event",   "ns", 1 },
        { "time-Allreduce", "Time spent in Allreduce event", "ns", 1 },
        { "time-Reduce", "Time spent in Reduce event",       "ns", 1 },
        { "time-Bcast", "Time spent in Bcast event",         "ns", 1 },
        { "time-Scatter", "Time spent in Scatter event",     "ns", 1 },
        { "time-Scatterv", "Time spent in Scatter event",    "ns", 1 },
        { "time-Gettime", "Time spent in Gettime event",     "ns", 1 },
        { "time-Commsplit", "Time spent in Commsplit event", "ns", 1 },
        { "time-Commcreate", "Time spent in Commcreate event", "ns", 1 },
    )

	EmberMessagePassingGenerator( ComponentId_t id, Params& params, std::string name = "" );
	~EmberMessagePassingGenerator();

    virtual void configure() override
    {
        mpi().setEventStatistics( m_mpiTimeStats );
    }

    virtual void completed( const SST::Output* output, uint64_t time ) override
    {
		mpi().completed(output, time, getMotifName(), getMotifNum());
	};

protected:

	EmberMpiLib& mpi() { return *m_mpi; }

	int rank() { return mpi().getRankCache(); }
	int size() { return mpi().getSizeCache(); }
	void setRank( int rank ) { mpi().setRankCache( rank );  }
	void setSize( int size ) { mpi().setSizeCache( size );  }

	void getPosition( int32_t rank, int32_t px, int32_t py, int32_t pz, int32_t* myX, int32_t* myY, int32_t* myZ) {
		m_rankMap->getPosition(rank, px, py, pz, myX, myY, myZ);
	}

	void getPosition( int32_t rank, int32_t px, int32_t py, int32_t* myX, int32_t* myY) {
		m_rankMap->getPosition(rank, px, py, myX, myY);
	}

	int32_t convertPositionToRank( int32_t px, int32_t py, int32_t pz, int32_t myX, int32_t myY, int32_t myZ) {
		return m_rankMap->convertPositionToRank(px, py, pz, myX, myY, myZ);
	}

	int32_t convertPositionToRank( int32_t px, int32_t py, int32_t myX, int32_t myY) {
		return m_rankMap->convertPositionToRank(px, py, myX, myY);
	}

	ReductionOperation op_create( User_function* func, int commute ) {
		return Hermes::MP::Op_create( func, commute );
	}

	void op_free( ReductionOperation op ) {
		Hermes::MP::Op_free( op );
	}

	int sizeofDataType( PayloadDataType type ) {
		return mpi().sizeofDataType( type );
	}

    int get_count( MessageResponse* resp, PayloadDataType datatype, int* count ) {
        uint32_t nbytes = resp->count * resp->dtypeSize;
        int dtypesize = sizeofDataType(datatype);
        if ( nbytes % dtypesize ) {
            *count = 0;
            return -1;
        }
        *count = nbytes / dtypesize;
        return 0;
    }

    EmberRankMap* getRankMap() { return m_rankMap; }

    void memSetBacked() override {
        EmberGenerator::memSetBacked();
        mpi().setBacked();
    }

    void memSetNotBacked() override {
        EmberGenerator::memSetNotBacked();
        mpi().setNotBacked();
    }

private:
    EmberMpiLib*	m_mpi;
    EmberRankMap*	m_rankMap;

    std::vector< EventTimeStat* > m_mpiTimeStats;
};


}
}

#endif
