// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_EMBER_MPI_GENERATOR
#define _H_EMBER_MPI_GENERATOR

#include <sst/core/stats/histo/histo.h>

#include "embergen.h"

#include <sst/elements/hermes/msgapi.h>

#include "emberconstdistrib.h"
#include "emberinitev.h"
#include "emberfinalizeev.h"
#include "emberbarrierev.h"
#include "emberrankev.h"
#include "embersizeev.h"
#include "embercomputeev.h"
#include "embersendev.h"
#include "emberrecvev.h"
#include "emberisendev.h"
#include "emberirecvev.h"
#include "embergettimeev.h"
#include "emberwaitallev.h"
#include "emberwaitev.h"
#include "emberCommSplitEv.h"
#include "emberallredev.h"
#include "emberredev.h"

using namespace Hermes::MP;
using namespace SST::Statistics;

namespace SST {
namespace Ember {

#undef FOREACH_ENUM
#define FOREACH_ENUM(NAME) \
    NAME( Init ) \
    NAME( Finalize ) \
    NAME( Rank ) \
    NAME( Size ) \
    NAME( Send ) \
    NAME( Recv ) \
    NAME( Irecv ) \
    NAME( Isend ) \
    NAME( Wait ) \
    NAME( Waitall ) \
    NAME( Waitany ) \
    NAME( Compute ) \
    NAME( Barrier ) \
    NAME( Allreduce ) \
    NAME( Reduce ) \
    NAME( Gettime ) \
    NAME( Commsplit ) \
    NAME( NUM_EVENTS ) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

class EmberSpyInfo {
    public:
        EmberSpyInfo(const int32_t rank);
        void incrementSendCount();
        void addSendBytes(uint64_t sendBytes);
        int32_t getRemoteRank();
        uint32_t getSendCount();
        uint64_t getBytesSent();

    private:
        const int32_t rank;
        uint64_t bytesSent;
        uint32_t sendsPerformed;
};

const uint32_t EMBER_SPYPLOT_NONE = 0;
const uint32_t EMBER_SPYPLOT_SEND_COUNT = 1;
const uint32_t EMBER_SPYPLOT_SEND_BYTES = 2;


class EmberMessagePassingGeneratorData : public EmberGeneratorData {
public:
	EmberMessagePassingGeneratorData() : size(0), rank(-1), jobId(-1) {}
    int                 size;
    uint32_t            rank;
    int                 jobId;
};

class EmberMessagePassingGenerator : public EmberGenerator {

public:

    enum Events {
        FOREACH_ENUM(GENERATE_ENUM)
    };

    typedef std::queue<EmberEvent*> Queue;

	EmberMessagePassingGenerator( Component* owner, Params& params );
	~EmberMessagePassingGenerator();
    virtual void finish( const SST::Output*, uint64_t time );

protected:

    void initData( EmberGeneratorData** data ) {
        assert( *data );
        m_data = static_cast<EmberMessagePassingGeneratorData*>(*data);
    }

	void getPosition( int32_t rank, int32_t px, int32_t py, int32_t pz, 
					int32_t* myX, int32_t* myY, int32_t* myZ );	
	void getPosition( int32_t rank, int32_t px, int32_t py,
                	int32_t* myX, int32_t* myY);
	int32_t convertPositionToRank( int32_t peX, int32_t peY, int32_t peZ, 
    	int32_t posX, int32_t posY, int32_t posZ);	

	int rank() { 
		assert(m_data); 
		return m_data->rank;
	}
	int size() { 
		assert(m_data); 
		return m_data->size;
	}
	void setRank( int rank ) {
		assert(m_data); 
		m_data->rank = rank;
	}

	void setSize( int size ) {
		assert(m_data); 
		m_data->size = size;
	}

	void setData( EmberMessagePassingGeneratorData* data ) {
		m_data = data;
	}
	EmberMessagePassingGeneratorData* getData() {
		return m_data;
	}

    inline void enQ_barrier( Queue&, Communicator );
    inline void enQ_fini( Queue& );
    inline void enQ_init( Queue& );
    inline void enQ_compute( Queue&, uint32_t nanoSecondDelay );
    inline void enQ_rank( Queue&, Communicator, uint32_t* rankPtr);
    inline void enQ_size( Queue&, Communicator, int* sizePtr);
    inline void enQ_send( Queue&, Addr payload, uint32_t count,
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group);
    inline void enQ_recv( Queue&, Addr payload, uint32_t count,
        PayloadDataType dtype, RankID src, uint32_t tag, Communicator group,
        MessageResponse* resp );
    inline void enQ_getTime( Queue&, uint64_t* time );
    inline void enQ_isend( Queue&, Addr payload, uint32_t count,
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group,
        MessageRequest* req );
    inline void enQ_irecv( Queue&, Addr target, uint32_t count,
        PayloadDataType dtype, RankID source, uint32_t tag, Communicator group,
        MessageRequest* req );

    inline void enQ_wait( Queue&, MessageRequest* req, 
									MessageResponse* resp = NULL );

    inline void enQ_waitall( Queue&, int count, MessageRequest req[],
                MessageResponse* resp[] );
    inline void enQ_commSplit( Queue&, Communicator, int color, int key, 
                Communicator* newComm );
    inline void enQ_allreduce( Queue&, Addr mydata, Addr result, uint32_t count,
                PayloadDataType dtype, ReductionOperation op,
                Communicator group );
    inline void enQ_reduce( Queue&, Addr mydata, Addr result, uint32_t count,
                PayloadDataType dtype, ReductionOperation op,
                int root, Communicator group );

	// deprecated
	inline void enQ_send( Queue&, uint32_t dst, uint32_t nBytes, int tag,
											Communicator);
	inline void enQ_recv( Queue&, uint32_t src, uint32_t nBytes, int tag,
											Communicator);

	inline void enQ_isend( Queue&, uint32_t dest, uint32_t nBytes, int tag,
									Communicator, MessageRequest* req );
	inline void enQ_irecv( Queue&, uint32_t src, uint32_t nBytes, int tag,
									Communicator, MessageRequest* req );


    void initOutput( ) {
        std::ostringstream prefix;
        prefix << "@t:" << m_data->jobId << ":" << (signed) m_data->rank 
                << ":EmberEngine:MPI:" << m_name << ":@p:@l: ";
        m_outputPrefix = prefix.str();
    }

private:

    void updateSpyplot( RankID remoteRank, size_t bytesSent);
    void generateSpyplotRank( const std::string filename );

    int                 m_printStats;
    uint32_t            m_spyplotMode;
    static const char*  m_eventName[];

    EmberMessagePassingGeneratorData* 	m_data;
	EmberRankMap* 						m_rankMap;
    EmberComputeDistribution*           m_computeDistrib; 
    std::vector< Histo* >               m_histoV;
	std::map< std::string, Histo*>      m_histoM;
    std::map<int32_t, EmberSpyInfo*>*   m_spyinfo;
};

static inline MP::Interface* cast( Hermes::Interface *in )
{
    return static_cast<MP::Interface*>(in);
}

void EmberMessagePassingGenerator::enQ_barrier( Queue& q, Communicator com )
{
    q.push( new EmberBarrierEvent( *cast(m_api), m_output,
                                    m_histoV[Barrier], com ) );
}

void EmberMessagePassingGenerator::enQ_init( Queue& q )
{
    q.push( new EmberInitEvent( *cast(m_api), m_output,
                                    m_histoV[Init] ) );
}

void EmberMessagePassingGenerator::enQ_fini( Queue& q )
{
    q.push( new EmberFinalizeEvent( *cast(m_api), m_output, 
                                    m_histoV[Finalize] ) );
}

void EmberMessagePassingGenerator::enQ_rank( Queue& q, Communicator comm,
                                    uint32_t* rankPtr )
{
    q.push( new EmberRankEvent( *cast(m_api), m_output, 
                                    m_histoV[Rank], comm, rankPtr ) );
}

void EmberMessagePassingGenerator::enQ_size( Queue& q, Communicator comm,
                                    int* sizePtr )
{
    q.push( new EmberSizeEvent( *cast(m_api), m_output, 
                                    m_histoV[Size], comm, sizePtr ) );
}

void EmberMessagePassingGenerator::enQ_compute( Queue& q, uint32_t delay )
{
    q.push( new EmberComputeEvent( m_output,
                                m_histoV[Compute], delay, m_computeDistrib ) );
}

void EmberMessagePassingGenerator::enQ_send( Queue& q, Addr payload,
    uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag,
    Communicator group)
{
	GEN_DBG(2,"dest=%d tag=%#x\n",dest,tag);
    q.push( new EmberSendEvent( *cast(m_api), m_output, m_histoV[Send],
		payload, count, dtype, dest, tag, group ) );

    size_t bytes = cast(m_api)->sizeofDataType(dtype);

	m_histoM["SendSize"]->add( count * bytes );

    if ( m_spyplotMode > EMBER_SPYPLOT_NONE ) {
        updateSpyplot( dest, bytes );
    }
}
void EmberMessagePassingGenerator::enQ_send( Queue& q, uint32_t dst,
						uint32_t nBytes, int tag, Communicator group )
{
	enQ_send( q, NULL, nBytes, CHAR, dst, tag, group );
}

void EmberMessagePassingGenerator::enQ_isend( Queue& q, Addr payload, 
    uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag, 
    Communicator group, MessageRequest* req )
{
	GEN_DBG(2,"dest=%d tag=%#x\n",dest,tag);
    q.push( new EmberISendEvent( *cast(m_api), m_output, m_histoV[Isend],
        payload, count, dtype, dest, tag, group, req ) );
    
    size_t bytes = cast(m_api)->sizeofDataType(dtype);

	m_histoM["SendSize"]->add( count * bytes );

    if ( m_spyplotMode > EMBER_SPYPLOT_NONE ) {
        updateSpyplot( dest, bytes );
    }
}

void EmberMessagePassingGenerator::enQ_isend( Queue& q, uint32_t dest,
		 uint32_t nBytes, int tag, Communicator group, MessageRequest* req )
{
	enQ_isend( q, NULL, nBytes, CHAR, dest, tag, group, req );
}

void EmberMessagePassingGenerator::enQ_recv( Queue& q, Addr payload,
    uint32_t count, PayloadDataType dtype, RankID src, uint32_t tag,
    Communicator group, MessageResponse* resp )
{
	GEN_DBG(2,"src=%d tag=%#x\n",src,tag);
    q.push( new EmberRecvEvent( *cast(m_api), m_output, m_histoV[Recv],
		payload, count, dtype, src, tag, group, resp ) );

	m_histoM["RecvSize"]->add( count * cast(m_api)->sizeofDataType(dtype) ); 
}

inline void EmberMessagePassingGenerator::enQ_recv( Queue& q, uint32_t src,
						uint32_t nBytes, int tag, Communicator group )
{
	enQ_recv( q, NULL, nBytes, CHAR, src, tag, group, NULL ); 
}

void EmberMessagePassingGenerator::enQ_irecv( Queue& q, Addr payload,
    uint32_t count, PayloadDataType dtype, RankID source, uint32_t tag,
    Communicator group, MessageRequest* req )
{
	GEN_DBG(2,"src=%d tag=%x\n",source,tag);
    q.push( new EmberIRecvEvent( *cast(m_api), m_output, m_histoV[Irecv],
        payload, count, dtype, source, tag, group, req ) );

	m_histoM["RecvSize"]->add( count * cast(m_api)->sizeofDataType(dtype) ); 
}

void EmberMessagePassingGenerator::enQ_irecv( Queue& q, uint32_t src,
	uint32_t nBytes, int tag, Communicator group, MessageRequest* req )
{
	enQ_irecv( q, NULL, nBytes, CHAR, src, tag, group, req );
}

void EmberMessagePassingGenerator::enQ_getTime( Queue& q, uint64_t* time )
{
    q.push( new EmberGetTimeEvent( m_output, time ) ); 
}

void EmberMessagePassingGenerator::enQ_wait( Queue& q, MessageRequest* req,
				MessageResponse* resp )
{
    q.push( new EmberWaitEvent( *cast(m_api), m_output, m_histoV[Wait],
		 												req, resp ) ); 
}

void EmberMessagePassingGenerator::enQ_waitall( Queue& q, int count,
    MessageRequest req[], MessageResponse* resp[] )
{
    q.push( new EmberWaitallEvent( *cast(m_api), m_output, m_histoV[Waitall],
        count, req, resp ) );
}
void EmberMessagePassingGenerator::enQ_commSplit( Queue& q, Communicator oldcom,
        int color, int key, Communicator* newCom )
{
    q.push( new EmberCommSplitEvent( *cast(m_api), m_output, 
        m_histoV[Commsplit], oldcom, color, key, newCom ) );
}

void EmberMessagePassingGenerator::enQ_allreduce( Queue& q, Addr mydata,
    Addr result, uint32_t count, PayloadDataType dtype, ReductionOperation op,
    Communicator group )
{
    q.push( new EmberAllreduceEvent( *cast(m_api), m_output, 
        m_histoV[Allreduce], mydata, result, count, dtype, op, group ) );
}

void EmberMessagePassingGenerator::enQ_reduce( Queue& q, Addr mydata,
    Addr result, uint32_t count, PayloadDataType dtype, ReductionOperation op,
    int root, Communicator group )
{
    q.push( new EmberReduceEvent( *cast(m_api), m_output, 
        m_histoV[Reduce], mydata, result, count, dtype, op, root, group ) );
}

}
}

#endif
