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

#ifndef _H_EMBER_MPI_GENERATOR
#define _H_EMBER_MPI_GENERATOR

#include <queue>

#include "embergen.h"

#include <sst/core/elementinfo.h>
#include <sst/elements/hermes/msgapi.h>

#include "emberMPIEvent.h"
#include "emberconstdistrib.h"
#include "emberinitev.h"
#include "embermakeprogressev.h"
#include "emberfinalizeev.h"
#include "emberalltoallvev.h"
#include "emberalltoallev.h"
#include "emberbarrierev.h"
#include "emberrankev.h"
#include "embersizeev.h"
#include "embersendev.h"
#include "emberrecvev.h"
#include "emberisendev.h"
#include "emberirecvev.h"
#include "embergettimeev.h"
#include "emberwaitallev.h"
#include "emberwaitev.h"
#include "emberCommSplitEv.h"
#include "emberCommCreateEv.h"
#include "emberCommDestroyEv.h"
#include "emberallredev.h"
#include "emberbcastev.h"
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
    NAME( Alltoallv ) \
    NAME( Alltoall ) \
    NAME( Allreduce ) \
    NAME( Reduce ) \
    NAME( Bcast) \
    NAME( Gettime ) \
    NAME( Commsplit ) \
    NAME( Commcreate ) \
    NAME( NUM_EVENTS ) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

class EmberSpyInfo {
  public:
	EmberSpyInfo(const int32_t rank) 
	  : rank(rank), bytesSent(0), sendsPerformed(0) { }

    void incrementSendCount() {sendsPerformed = sendsPerformed + 1; }
    void addSendBytes(uint64_t sendBytes) { bytesSent += sendBytes; }
    int32_t getRemoteRank() { return rank; }
        uint32_t getSendCount() { return sendsPerformed; }
        uint64_t getBytesSent() { return bytesSent; }

    private:
        const int32_t rank;
        uint64_t bytesSent;
        uint32_t sendsPerformed;
};

const uint32_t EMBER_SPYPLOT_NONE = 0;
const uint32_t EMBER_SPYPLOT_SEND_COUNT = 1;
const uint32_t EMBER_SPYPLOT_SEND_BYTES = 2;

class EmberMessagePassingGenerator : public EmberGenerator {

public:

    enum Events {
        FOREACH_ENUM(GENERATE_ENUM)
    };

	EmberMessagePassingGenerator( Component* owner, Params& params, std::string name = "" );
	~EmberMessagePassingGenerator();
    virtual void completed( const SST::Output*, uint64_t time );

	void getPosition( int32_t rank, int32_t px, int32_t py, int32_t pz, 
					int32_t* myX, int32_t* myY, int32_t* myZ );	
	void getPosition( int32_t rank, int32_t px, int32_t py,
                	int32_t* myX, int32_t* myY);
	int32_t convertPositionToRank( int32_t px, int32_t py, 
    	int32_t myX, int32_t myY);	
	int32_t convertPositionToRank( int32_t px, int32_t py, int32_t pz, 
    	int32_t myX, int32_t myY, int32_t myZ);	

    inline void enQ_barrier( Queue&, Communicator );
    inline void enQ_fini( Queue& );
    inline void enQ_init( Queue& );
    inline void enQ_rank( Queue&, Communicator, uint32_t* rankPtr);
    inline void enQ_size( Queue&, Communicator, int* sizePtr);
    inline void enQ_makeProgress( Queue& );


    inline void enQ_send( Queue&, Addr payload, uint32_t count,
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group);
    inline void enQ_send(Queue&, const Hermes::MemAddr& payload, uint32_t count,
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group);

    inline void enQ_recv( Queue&, Addr payload, uint32_t count,
        PayloadDataType dtype, RankID src, uint32_t tag, Communicator group,
        MessageResponse* resp = NULL );
    inline void enQ_recv(Queue&, const Hermes::MemAddr& payload, uint32_t count,
        PayloadDataType dtype, RankID src, uint32_t tag, Communicator group,
        MessageResponse* resp = NULL );

    inline void enQ_getTime( Queue&, uint64_t* time );

    inline void enQ_isend( Queue&, Addr payload, uint32_t count,
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group,
        MessageRequest* req );
    inline void enQ_isend( Queue&, const Hermes::MemAddr& payload, uint32_t count,
        PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group,
        MessageRequest* req );

    inline void enQ_irecv( Queue&, Addr target, uint32_t count,
        PayloadDataType dtype, RankID source, uint32_t tag, Communicator group,
        MessageRequest* req );
    inline void enQ_irecv( Queue&, const Hermes::MemAddr& target, uint32_t count,
        PayloadDataType dtype, RankID source, uint32_t tag, Communicator group,
        MessageRequest* req );

    inline void enQ_wait( Queue&, MessageRequest* req, 
									MessageResponse* resp = NULL );
    inline void enQ_waitall( Queue&, int count, MessageRequest req[],
                MessageResponse* resp[] = NULL );

    inline void enQ_commSplit( Queue&, Communicator, int color, int key, 
                Communicator* newComm );
    inline void enQ_commCreate( Queue&, Communicator, std::vector<int>&,
                Communicator* newComm );
    inline void enQ_commDestroy( Queue&, Communicator );

    inline void enQ_allreduce( Queue&, Addr mydata, Addr result, uint32_t count,
                PayloadDataType dtype, ReductionOperation op,
                Communicator group );
    inline void enQ_allreduce( Queue&, const Hermes::MemAddr& mydata,
				const Hermes::MemAddr& result, uint32_t count,
                PayloadDataType dtype, ReductionOperation op,
                Communicator group );

    inline void enQ_reduce( Queue&, Addr mydata, Addr result, uint32_t count,
                PayloadDataType dtype, ReductionOperation op,
                int root, Communicator group );
    inline void enQ_reduce( Queue&, const Hermes::MemAddr& mydata,
				const Hermes::MemAddr& result, uint32_t count,
                PayloadDataType dtype, ReductionOperation op,
                int root, Communicator group );

    inline void enQ_bcast( Queue&, Addr mydata, uint32_t count,
                PayloadDataType dtype, int root, Communicator group );
    inline void enQ_bcast( Queue&, const Hermes::MemAddr& mydata, uint32_t count,
                PayloadDataType dtype, int root, Communicator group );

    inline void enQ_alltoall( Queue&, 
        Addr sendData, int sendCnts, PayloadDataType senddtype,
        Addr recvData, int recvCnts, PayloadDataType recvdtype,
        Communicator group );
    inline void enQ_alltoall( Queue&, 
        const Hermes::MemAddr& sendData, int sendCnts, PayloadDataType senddtype,
        const Hermes::MemAddr& recvData, int recvCnts, PayloadDataType recvdtype,
        Communicator group );

    inline void enQ_alltoallv( Queue&, 
        Addr sendData, Addr sendCnts, Addr sendDsp, PayloadDataType senddtype,
        Addr recvData, Addr recvCnts, Addr recvDsp, PayloadDataType recvdtype,
        Communicator group );
    inline void enQ_alltoallv( Queue&, 
        const Hermes::MemAddr& sendData, Addr sendCnts, Addr sendDsp, PayloadDataType senddtype,
        const Hermes::MemAddr& recvData, Addr recvCnts, Addr recvDsp, PayloadDataType recvdtype,
        Communicator group );

    inline int sizeofDataType( PayloadDataType );

	// deprecated
	inline void enQ_send( Queue&, uint32_t dst, uint32_t nBytes, int tag,
											Communicator);
	inline void enQ_recv( Queue&, uint32_t src, uint32_t nBytes, int tag,
											Communicator);

	inline void enQ_isend( Queue&, uint32_t dest, uint32_t nBytes, int tag,
									Communicator, MessageRequest* req );
	inline void enQ_irecv( Queue&, uint32_t src, uint32_t nBytes, int tag,
									Communicator, MessageRequest* req );
    EmberRankMap*                       m_rankMap;

private:

    void updateSpyplot( RankID remoteRank, size_t bytesSent );
    void generateSpyplotRank( const char* filename );

    uint32_t            m_spyplotMode;
    static const char*  m_eventName[];
    std::vector< Statistic<uint32_t>* > m_Stats;
    std::map<int32_t, EmberSpyInfo*>*   m_spyinfo;
};

static inline MP::Interface* cast( Hermes::Interface *in )
{
    return static_cast<MP::Interface*>(in);
}

int EmberMessagePassingGenerator::sizeofDataType( PayloadDataType type )
{
    return cast(m_api)->sizeofDataType( type );
}

void EmberMessagePassingGenerator::enQ_barrier( Queue& q, Communicator com )
{
    q.push( new EmberBarrierEvent( *cast(m_api), &getOutput(),
                                    m_Stats[Barrier], com ) );
}

void EmberMessagePassingGenerator::enQ_init( Queue& q )
{
    q.push( new EmberInitEvent( *cast(m_api), &getOutput(),
                                    m_Stats[Init] ) );
}

void EmberMessagePassingGenerator::enQ_fini( Queue& q )
{
    q.push( new EmberFinalizeEvent( *cast(m_api), &getOutput(), 
                                    m_Stats[Finalize] ) );
}

void EmberMessagePassingGenerator::enQ_rank( Queue& q, Communicator comm,
                                    uint32_t* rankPtr )
{
    q.push( new EmberRankEvent( *cast(m_api), &getOutput(), 
                                    m_Stats[Rank], comm, rankPtr ) );
}

void EmberMessagePassingGenerator::enQ_size( Queue& q, Communicator comm,
                                    int* sizePtr )
{
    q.push( new EmberSizeEvent( *cast(m_api), &getOutput(), 
                                    m_Stats[Size], comm, sizePtr ) );
}

void EmberMessagePassingGenerator::enQ_makeProgress( Queue& q )
{
    q.push( new EmberMakeProgressEvent( *cast(m_api), &getOutput(),
                                    m_Stats[Init] ) );
}

void EmberMessagePassingGenerator::enQ_send( Queue& q, Addr payload,
    uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag,
    Communicator group)
{
	Hermes::MemAddr addr( memAddr(payload) );

	enQ_send( q, addr, count, dtype, dest, tag, group );
}
void EmberMessagePassingGenerator::enQ_send( Queue& q, 
	const Hermes::MemAddr& payload,
    uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag,
    Communicator group)
{
	verbose(CALL_INFO,2,0,"payload=0x%" PRIx64 " dest=%d tag=%#x\n",(uint64_t) &payload, dest, tag);
    q.push( new EmberSendEvent( *cast(m_api), &getOutput(), m_Stats[Send],
		payload, count, dtype, dest, tag, group ) );

    size_t bytes = cast(m_api)->sizeofDataType(dtype);

	//m_histoM["SendSize"]->add( count * bytes );

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
	Hermes::MemAddr addr( memAddr(payload) );
	enQ_isend(q,addr,count,dtype,dest,tag,group,req);
}
void EmberMessagePassingGenerator::enQ_isend( Queue& q, 
	const Hermes::MemAddr& payload, 
    uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag, 
    Communicator group, MessageRequest* req )
{
	verbose(CALL_INFO,2,0,"payload=0x%" PRIx64" dest=%d tag=%#x req=%p\n",(uint64_t)&payload, dest, tag, req );
    q.push( new EmberISendEvent( *cast(m_api), &getOutput(), m_Stats[Isend],
        payload, count, dtype, dest, tag, group, req ) );
    
    size_t bytes = cast(m_api)->sizeofDataType(dtype);

	//m_histoM["SendSize"]->add( count * bytes );

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
	Hermes::MemAddr addr( memAddr(payload) );
	enQ_recv( q, addr, count, dtype, src, tag, group, resp );
}
void EmberMessagePassingGenerator::enQ_recv( Queue& q,
	const Hermes::MemAddr& payload,
    uint32_t count, PayloadDataType dtype, RankID src, uint32_t tag,
    Communicator group, MessageResponse* resp )
{
	verbose(CALL_INFO,2,0,"src=%d tag=%#x\n",src,tag);
    q.push( new EmberRecvEvent( *cast(m_api), &getOutput(), m_Stats[Recv],
		payload, count, dtype, src, tag, group, resp ) );
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
	Hermes::MemAddr addr( memAddr(payload) );
	enQ_irecv( q, addr, count, dtype, source, tag, group, req );
}

void EmberMessagePassingGenerator::enQ_irecv( Queue& q,
	const Hermes::MemAddr& payload,
    uint32_t count, PayloadDataType dtype, RankID source, uint32_t tag,
    Communicator group, MessageRequest* req )
{
	verbose(CALL_INFO,2,0,"src=%d tag=%x req=%p\n",source,tag,req);
    q.push( new EmberIRecvEvent( *cast(m_api), &getOutput(), m_Stats[Irecv],
        payload, count, dtype, source, tag, group, req ) );
}

void EmberMessagePassingGenerator::enQ_irecv( Queue& q, uint32_t src,
	uint32_t nBytes, int tag, Communicator group, MessageRequest* req )
{
	enQ_irecv( q, NULL, nBytes, CHAR, src, tag, group, req );
}

void EmberMessagePassingGenerator::enQ_getTime( Queue& q, uint64_t* time )
{
    q.push( new EmberGetTimeEvent( &getOutput(), time ) ); 
}

void EmberMessagePassingGenerator::enQ_wait( Queue& q, MessageRequest* req,
				MessageResponse* resp )
{
    q.push( new EmberWaitEvent( *cast(m_api), &getOutput(), m_Stats[Wait],
		 												req, resp ) ); 
}

void EmberMessagePassingGenerator::enQ_waitall( Queue& q, int count,
    MessageRequest req[], MessageResponse* resp[] )
{
    q.push( new EmberWaitallEvent( *cast(m_api), &getOutput(), m_Stats[Waitall],
        count, req, resp ) );
}
void EmberMessagePassingGenerator::enQ_commSplit( Queue& q, Communicator oldcom,
        int color, int key, Communicator* newCom )
{
    q.push( new EmberCommSplitEvent( *cast(m_api), &getOutput(), 
        m_Stats[Commsplit], oldcom, color, key, newCom ) );
}

void EmberMessagePassingGenerator::enQ_commCreate( Queue& q, 
        Communicator oldcom, std::vector<int>& ranks, Communicator* newCom )
{
    q.push( new EmberCommCreateEvent( *cast(m_api), &getOutput(), 
        m_Stats[Commsplit], oldcom, ranks, newCom ) );
}

void EmberMessagePassingGenerator::enQ_commDestroy( Queue& q, 
            Communicator comm )
{
    q.push( new EmberCommDestroyEvent( *cast(m_api), &getOutput(), 
        m_Stats[Commsplit], comm ) );
}

void EmberMessagePassingGenerator::enQ_allreduce( Queue& q, Addr _mydata,
    Addr _result, uint32_t count, PayloadDataType dtype, ReductionOperation op,
    Communicator group )
{
	Hermes::MemAddr mydata( memAddr( _mydata ) );
	Hermes::MemAddr result( memAddr( _result ) );
	enQ_allreduce( q, mydata, result, count, dtype, op, group );
}
	
void EmberMessagePassingGenerator::enQ_allreduce( Queue& q, 
	const Hermes::MemAddr& mydata,
    const Hermes::MemAddr& result,
	uint32_t count, PayloadDataType dtype, ReductionOperation op,
    Communicator group )
{
    q.push( new EmberAllreduceEvent( *cast(m_api), &getOutput(), 
        m_Stats[Allreduce], mydata, result, count, dtype, op, group ) );
}

void EmberMessagePassingGenerator::enQ_reduce( Queue& q, Addr _mydata,
    Addr _result, uint32_t count, PayloadDataType dtype, ReductionOperation op,
    int root, Communicator group )
{
	Hermes::MemAddr mydata( memAddr( _mydata ) );
	Hermes::MemAddr result( memAddr( _result ) );
	enQ_reduce( q, mydata, result, count, dtype, op, root, group );
}

void EmberMessagePassingGenerator::enQ_reduce( Queue& q, 
	const Hermes::MemAddr& mydata,
    const Hermes::MemAddr& result,
	uint32_t count, PayloadDataType dtype, ReductionOperation op,
    int root, Communicator group )
{
    q.push( new EmberReduceEvent( *cast(m_api), &getOutput(), 
        m_Stats[Reduce], mydata, result, count, dtype, op, root, group ) );
}

void EmberMessagePassingGenerator::enQ_bcast( Queue& q, Addr mydata,
    uint32_t count, PayloadDataType dtype, int root, Communicator group )
{
	Hermes::MemAddr addr( memAddr(mydata) );
	enQ_bcast( q, addr, count, dtype, root, group );
}
void EmberMessagePassingGenerator::enQ_bcast( Queue& q, 
	const Hermes::MemAddr& mydata,
    uint32_t count, PayloadDataType dtype, int root, Communicator group )
{
    q.push( new EmberBcastEvent( *cast(m_api), &getOutput(), 
        m_Stats[Reduce], mydata, count, dtype, root, group ) );
}

void EmberMessagePassingGenerator::enQ_alltoall( Queue& q, 
        Addr _sendData, int sendCnts, PayloadDataType senddtype,
        Addr _recvData, int recvCnts, PayloadDataType recvdtype,
        Communicator group )
{
	Hermes::MemAddr sendData( memAddr( _sendData ) );
	Hermes::MemAddr recvData( memAddr( _recvData ) );
	enQ_alltoall( q, sendData, sendCnts, senddtype,
				recvData, recvCnts, recvdtype, group );
}

void EmberMessagePassingGenerator::enQ_alltoall( Queue& q, 
    const Hermes::MemAddr& sendData, int sendCnts, PayloadDataType senddtype,
    const Hermes::MemAddr& recvData, int recvCnts, PayloadDataType recvdtype,
        Communicator group )
{
    q.push( new EmberAlltoallEvent( *cast(m_api), &getOutput(), 
        m_Stats[Alltoall], sendData, sendCnts, senddtype,
        recvData, recvCnts, recvdtype, group ) );
}

void EmberMessagePassingGenerator::enQ_alltoallv( Queue& q, 
        Addr _sendData, Addr sendCnts, Addr sendDsp, PayloadDataType senddtype,
        Addr _recvData, Addr recvCnts, Addr recvDsp, PayloadDataType recvdtype,
        Communicator group )
{
	Hermes::MemAddr sendData( memAddr( _sendData ) );
	Hermes::MemAddr recvData( memAddr( _recvData ) );
	enQ_alltoallv( q, sendData, sendCnts, sendDsp, senddtype,
					  recvData, recvCnts, recvDsp, recvdtype, group );
}
void EmberMessagePassingGenerator::enQ_alltoallv( Queue& q, 
        const Hermes::MemAddr& sendData,
		Addr sendCnts, Addr sendDsp, PayloadDataType senddtype,
        const Hermes::MemAddr& recvData,
		Addr recvCnts, Addr recvDsp, PayloadDataType recvdtype,
        Communicator group )
{
    q.push( new EmberAlltoallvEvent( *cast(m_api), &getOutput(), 
        m_Stats[Alltoallv],
            sendData, sendCnts, sendDsp, senddtype, 
            recvData, recvCnts, recvDsp, recvdtype, 
                group ) );
}

}
}

#endif
