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

#ifndef _H_EMBER_MPI_LIB
#define _H_EMBER_MPI_LIB

#include <queue>

#include "libs/emberLib.h"

#include "sst/elements/hermes/msgapi.h"

#include "mpi/emberMPIEvent.h"
#include "mpi/emberinitev.h"
#include "mpi/embermakeprogressev.h"
#include "mpi/emberfinalizeev.h"
#include "mpi/emberalltoallvev.h"
#include "mpi/emberalltoallev.h"
#include "mpi/emberallgatherev.h"
#include "mpi/emberallgathervev.h"
#include "mpi/emberbarrierev.h"
#include "mpi/emberrankev.h"
#include "mpi/embersizeev.h"
#include "mpi/embersendev.h"
#include "mpi/emberrecvev.h"
#include "mpi/emberisendev.h"
#include "mpi/emberirecvev.h"
#include "mpi/emberwaitallev.h"
#include "mpi/emberwaitanyev.h"
#include "mpi/emberwaitev.h"
#include "mpi/embercancelev.h"
#include "mpi/embertestev.h"
#include "mpi/embertestanyev.h"
#include "mpi/emberCommSplitEv.h"
#include "mpi/emberCommCreateEv.h"
#include "mpi/emberCommDestroyEv.h"
#include "mpi/emberallredev.h"
#include "mpi/emberbcastev.h"
#include "mpi/emberscatterev.h"
#include "mpi/emberscattervev.h"
#include "mpi/emberredev.h"

using namespace Hermes;
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
    NAME( Scatter) \
    NAME( Scatterv) \
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

class EmberMpiLib : public EmberLib {

    enum Events {
        FOREACH_ENUM(GENERATE_ENUM)
    };

  public:

    SST_ELI_REGISTER_MODULE(
        EmberMpiLib,
        "ember",
        "mpiLib",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        "SST::Ember::EmberMpiLib"
    )

    SST_ELI_DOCUMENT_PARAMS(
		{ "spyplotmode", "Sets the spyplot generation mode, 0 = none, 1 = spy on sends", "0" },
	)

    typedef std::queue<EmberEvent*> Queue;

	EmberMpiLib( Params& params );
	~EmberMpiLib() {
    	if ( m_spyinfo ) {
        	delete m_spyinfo;
		}
	}

    void init( Queue& q ) {
		q.push( new EmberInitEvent( api(), m_output, m_Stats[Init] ) );
	}
    void fini( Queue& q ) {
		q.push( new EmberFinalizeEvent( api(), m_output, m_Stats[Finalize] ) );
	}
    void rank( Queue& q, Communicator comm, uint32_t* rankPtr) {
		q.push( new EmberRankEvent( api(), m_output, m_Stats[Rank], comm, rankPtr ) );
	}
    void size( Queue& q, Communicator comm, int* sizePtr) {
		q.push( new EmberSizeEvent( api(), m_output, m_Stats[Size], comm, sizePtr ) );
	}
    void makeProgress( Queue& q ) {
		q.push( new EmberMakeProgressEvent( api(), m_output, m_Stats[Init] ) );
	}
    void barrier( Queue& q, Communicator comm ) {
		q.push( new EmberBarrierEvent( api(), m_output, m_Stats[Barrier], comm ) );
	}
    void send(Queue& q, const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group) {
    	q.push( new EmberSendEvent( api(), m_output, m_Stats[Send], payload, count, dtype, dest, tag, group ) );

    	size_t bytes = api().sizeofDataType(dtype);

	    //m_histoM["SendSize"]->add( count * bytes );

		if ( m_spyplotMode > EMBER_SPYPLOT_NONE ) {
			updateSpyplot( dest, bytes );
		}
	}
    void isend( Queue& q, const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group,
        MessageRequest* req ) {
    	q.push( new EmberISendEvent( api(), m_output, m_Stats[Isend], payload, count, dtype, dest, tag, group, req ) );

		size_t bytes = api().sizeofDataType(dtype);

		//m_histoM["SendSize"]->add( count * bytes );

		if ( m_spyplotMode > EMBER_SPYPLOT_NONE ) {
			updateSpyplot( dest, bytes );
		}
	}

    void recv(Queue& q, const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype, RankID src, uint32_t tag, Communicator group,
		   	MessageResponse* resp = NULL )
	{
		q.push( new EmberRecvEvent( api(), m_output, m_Stats[Recv], payload, count, dtype, src, tag, group, resp ) );
	}
    void irecv( Queue& q, const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype, RankID source, uint32_t tag, Communicator group,
        MessageRequest* req ) {
		q.push( new EmberIRecvEvent( api(), m_output, m_Stats[Irecv], payload, count, dtype, source, tag, group, req ) );
	}

    void cancel( Queue& q, MessageRequest req ) {
		q.push( new EmberCancelEvent( api(), m_output, m_Stats[Waitall], req ) );
	}

    void test( Queue& q, MessageRequest* req, int* flag, MessageResponse* resp = NULL ) {
		*flag = 0;
		q.push( new EmberTestEvent( api(), m_output, m_Stats[Waitall], req, flag, resp ) );
	}
    void testany( Queue& q, int count, MessageRequest req[], int* indx, int* flag, MessageResponse* resp = NULL ) {
		*flag = 0;
		q.push( new EmberTestanyEvent( api(), m_output, m_Stats[Waitall], count, req, indx, flag, resp ) );
	}
    void wait( Queue& q, MessageRequest* req, MessageResponse* resp = NULL ) {
		q.push( new EmberWaitEvent( api(), m_output, m_Stats[Wait], req, resp, false ) );
	}
    void waitall( Queue& q, int count, MessageRequest req[], MessageResponse* resp[] = NULL ) {
		q.push( new EmberWaitallEvent( api(), m_output, m_Stats[Waitall], count, req, resp ) );
	}

    void waitany( Queue& q, int count, MessageRequest req[], int *indx, MessageResponse* resp = NULL ) {
		q.push( new EmberWaitanyEvent( api(), m_output, m_Stats[Waitall],
        count, req, indx, resp ) );
	}

    void commSplit( Queue& q, Communicator oldcom, int color, int key, Communicator* newCom ) {
		q.push( new EmberCommSplitEvent( api(), m_output, m_Stats[Commsplit], oldcom, color, key, newCom ) );
	}
    void commCreate( Queue& q, Communicator oldcom, std::vector<int>& ranks, Communicator* newCom ) {
		q.push( new EmberCommCreateEvent( api(), m_output, m_Stats[Commsplit], oldcom, ranks, newCom ) );
	}
    void commDestroy( Queue& q, Communicator comm ) {
		q.push( new EmberCommDestroyEvent( api(), m_output, m_Stats[Commsplit], comm ) );
	}

    void allreduce( Queue& q, const Hermes::MemAddr& mydata, const Hermes::MemAddr& result, uint32_t count,
                PayloadDataType dtype, ReductionOperation op, Communicator group ) {
		q.push( new EmberAllreduceEvent( api(), m_output, m_Stats[Allreduce], mydata, result, count, dtype, op, group ) );
	}

    void reduce( Queue& q, const Hermes::MemAddr& mydata, const Hermes::MemAddr& result, uint32_t count,
                PayloadDataType dtype, ReductionOperation op, int root, Communicator group ) {
		q.push( new EmberReduceEvent( api(), m_output, m_Stats[Reduce], mydata, result, count, dtype, op, root, group ) );
	}

    void bcast( Queue& q, const Hermes::MemAddr& mydata, uint32_t count, PayloadDataType dtype, int root, Communicator group ) {
		q.push( new EmberBcastEvent( api(), m_output, m_Stats[Bcast], mydata, count, dtype, root, group ) );
	}

    void scatter( Queue& q, const Hermes::MemAddr& senddata, uint32_t sendCnt, PayloadDataType sendType,
			const Hermes::MemAddr& recvdata, uint32_t recvCnt, PayloadDataType recvType, int root, Communicator group ) {
		q.push( new EmberScatterEvent( api(), m_output, m_Stats[Scatter], senddata, sendCnt, sendType, recvdata, recvCnt, recvType, root, group ) );
	}

    void scatterv( Queue& q, const Hermes::MemAddr& senddata, int* sendCnts, int* displs, PayloadDataType sendType,
			const Hermes::MemAddr& recvdata, uint32_t recvCnt, PayloadDataType recvType, int root, Communicator group ) {
		q.push( new EmberScattervEvent( api(), m_output, m_Stats[Scatterv], senddata, sendCnts, displs, sendType, recvdata, recvCnt, recvType, root, group ) );
	}

    void allgather( Queue& q, const Hermes::MemAddr& sendData, int sendCnts, PayloadDataType senddtype,
        const Hermes::MemAddr& recvData, int recvCnts, PayloadDataType recvdtype, Communicator group )
	{
		q.push( new EmberAllgatherEvent( api(), m_output, m_Stats[Alltoall], sendData, sendCnts, senddtype, recvData, recvCnts, recvdtype, group ) );
	}

    void allgatherv( Queue& q, const Hermes::MemAddr& sendData, int sendCnts, PayloadDataType senddtype,
        const Hermes::MemAddr& recvData, Addr recvCnts, Addr recvDsp, PayloadDataType recvdtype, Communicator group )
	{
		q.push( new EmberAllgathervEvent( api(), m_output, m_Stats[Alltoallv],
			sendData, sendCnts, senddtype,
			recvData, recvCnts, recvDsp, recvdtype,
			group ) );
	}

    void alltoall( Queue& q, const Hermes::MemAddr& sendData, int sendCnts, PayloadDataType senddtype,
        const Hermes::MemAddr& recvData, int recvCnts, PayloadDataType recvdtype, Communicator group )
	{
		q.push( new EmberAlltoallEvent( api(), m_output, m_Stats[Alltoall], sendData, sendCnts, senddtype, recvData, recvCnts, recvdtype, group ) );
	}

    void alltoallv( Queue& q, const Hermes::MemAddr& sendData, Addr sendCnts, Addr sendDsp, PayloadDataType senddtype,
        const Hermes::MemAddr& recvData, Addr recvCnts, Addr recvDsp, PayloadDataType recvdtype, Communicator group )
	{
    	q.push( new EmberAlltoallvEvent( api(), m_output, m_Stats[Alltoallv],
			sendData, sendCnts, sendDsp, senddtype,
			recvData, recvCnts, recvDsp, recvdtype,
			group ) );
	}

    void sendrecv( Queue& q,
            Addr sendbuf, uint32_t sendcount, PayloadDataType sendtype,
            RankID dest, uint32_t sendtag,
            Addr recvbuf, uint32_t recvcnt, PayloadDataType recvtype,
            RankID source, uint32_t recvtag,
            Communicator group, MessageResponse* resp )
	{
    	MessageRequest* req = new MessageRequest;
    	irecv(q, recvbuf, recvcnt, recvtype, source, recvtag, group, req );
    	send(q, sendbuf, sendcount, sendtype, dest, sendtag, group );
    	q.push( new EmberWaitEvent( api(), m_output, m_Stats[Wait], req, resp, true ) );
	}

    void allreduce( Queue& q, Addr _mydata, Addr _result, uint32_t count, PayloadDataType dtype, ReductionOperation op, Communicator group ) {
		Hermes::MemAddr mydata( memAddr( _mydata ) );
		Hermes::MemAddr result( memAddr( _result ) );
    	allreduce( q, mydata, result, count, dtype, op, group );
	}
    void reduce( Queue& q, Addr _mydata, Addr _result, uint32_t count, PayloadDataType dtype, ReductionOperation op,
                int root, Communicator group )
	{
		Hermes::MemAddr mydata( memAddr( _mydata ) );
		Hermes::MemAddr result( memAddr( _result ) );
		reduce( q, mydata, result, count, dtype, op, root, group );
	}

    void bcast( Queue& q, Addr mydata, uint32_t count, PayloadDataType dtype, int root, Communicator group ) {
		Hermes::MemAddr addr( memAddr(mydata) );
		bcast( q, addr, count, dtype, root, group );
	}

	void alltoall( Queue& q, Addr _sendData, int sendCnts, PayloadDataType senddtype,
        Addr _recvData, int recvCnts, PayloadDataType recvdtype, Communicator group )
	{
		Hermes::MemAddr sendData( memAddr( _sendData ) );
		Hermes::MemAddr recvData( memAddr( _recvData ) );
		alltoall( q, sendData, sendCnts, senddtype, recvData, recvCnts, recvdtype, group );
	}

    void alltoallv( Queue& q, Addr _sendData, Addr sendCnts, Addr sendDsp, PayloadDataType senddtype,
        Addr _recvData, Addr recvCnts, Addr recvDsp, PayloadDataType recvdtype, Communicator group )
	{
		Hermes::MemAddr sendData( memAddr( _sendData ) );
		Hermes::MemAddr recvData( memAddr( _recvData ) );
		alltoallv( q, sendData, sendCnts, sendDsp, senddtype, recvData, recvCnts, recvDsp, recvdtype, group );
	}
    void send( Queue& q, Addr payload, uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group) {
    	Hermes::MemAddr addr( memAddr(payload) );
    	send( q, addr, count, dtype, dest, tag, group );
	}
    void isend( Queue& q, Addr payload, uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group, MessageRequest* req ) {
		Hermes::MemAddr addr( memAddr(payload) );
		isend(q,addr,count,dtype,dest,tag,group,req);
	}
    void recv( Queue& q, Addr payload, uint32_t count, PayloadDataType dtype, RankID src, uint32_t tag, Communicator group, MessageResponse* resp = NULL ) {
		Hermes::MemAddr addr( memAddr(payload) );
		recv( q, addr, count, dtype, src, tag, group, resp );
	}
    void irecv( Queue& q, Addr payload, uint32_t count, PayloadDataType dtype, RankID source, uint32_t tag, Communicator group, MessageRequest* req ) {
	    Hermes::MemAddr addr( memAddr(payload) );
    	irecv( q, addr, count, dtype, source, tag, group, req );
	}

    // deprecated
    void send( Queue& q, uint32_t dst, uint32_t nBytes, int tag, Communicator group ) {
		send( q, NULL, nBytes, CHAR, dst, tag, group );
	}
    void recv( Queue& q, uint32_t src, uint32_t nBytes, int tag, Communicator group) {
		recv( q, NULL, nBytes, CHAR, src, tag, group, NULL );
	}

    void isend( Queue& q, uint32_t dest, uint32_t nBytes, int tag, Communicator group, MessageRequest* req ) {
		isend( q, NULL, nBytes, CHAR, dest, tag, group, req );
	}
    void irecv( Queue& q, uint32_t src, uint32_t nBytes, int tag, Communicator group, MessageRequest* req ) {
		irecv( q, NULL, nBytes, CHAR, src, tag, group, req );
	}

	int sizeofDataType( PayloadDataType type ) {
        return api().sizeofDataType( type );
    }

	void  setSizeCache( int size ) {
		m_size = size;
	}
	int  getSizeCache() {
		return m_size;
	}
	void  setRankCache(int rank ) {
		m_rank = rank;
	}
	int  getRankCache() {
		return m_rank;
	}
	void setBacked() {
		m_backed = true;
	}

	void completed(const SST::Output* output, uint64_t time, std::string motifName, int motifNum );

  private:
    virtual void* memAddr( void * addr ) {
        return  m_backed ? addr : NULL;
    }

	MP::Interface& api() { return *static_cast<MP::Interface*>(m_api); }
	std::vector< Statistic<uint32_t>* > m_Stats;
	static const char*  m_eventName[];

	bool m_backed;
	int m_size;
	int m_rank;

    void updateSpyplot( RankID remoteRank, size_t bytesSent );
    void generateSpyplotRank( const char* filename );

    uint32_t            m_spyplotMode;
    std::map<int32_t, EmberSpyInfo*>*   m_spyinfo;
};

}
}

#endif
