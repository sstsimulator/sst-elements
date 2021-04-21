// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
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

#ifndef _SWM_CONVERT_H
#define _SWM_CONVERT_H

#include <sst/core/output.h>
#include <sst/elements/hermes/msgapi.h>
#include "sst/elements/swm/event.h"
#include <swm-include.h>

#if 0
extern int g_verboseLevel;
#define ConvertDBG( level, rank, format, ... ) \
	if ( level <= g_verboseLevel ) printf( "%d:Convert::%s():%d " format, rank, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ConvertDBG( level, rank, format, ... ) 
#endif

using namespace SST::Hermes::MP;
using namespace SST::Hermes;

namespace SST {
namespace Swm {

#define FOREACH_FUNCTION(NAME) \
    NAME(Empty) \
    NAME(Init) \
    NAME(Send) \
    NAME(Isend) \
    NAME(Recv) \
    NAME(Irecv) \
    NAME(SendRecv) \
    NAME(Allreduce) \
    NAME(Barrier) \
    NAME(Wait) \
    NAME(Waitall) \
    NAME(Finalize) \
    NAME(Compute)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

class Convert {

    static const char *m_functionName[];
  public:
	Convert( Link*, MP::Interface* mp, int rank, uint32_t verboseLevel, uint32_t verboseMask);

    void waitForWork();
    void MP_returned(int retval, int type );

    void init();
	void send(SWM_PEER peer, SWM_COMM_ID comm_id, SWM_TAG tag, SWM_VC reqvc, SWM_VC rspvc, SWM_BUF buf, SWM_BYTES bytes,
        SWM_BYTES pktrspbytes, SWM_ROUTING_TYPE reqrt, SWM_ROUTING_TYPE rsprt);
	void isend(SWM_PEER peer, SWM_COMM_ID comm_id, SWM_TAG tag, SWM_VC reqvc, SWM_VC rspvc, SWM_BUF buf, SWM_BYTES bytes,
        SWM_BYTES pktrspbytes, uint32_t * handle, SWM_ROUTING_TYPE reqrt, SWM_ROUTING_TYPE rsprt );
	void sendrecv( SWM_COMM_ID comm_id, SWM_PEER sendpeer, SWM_TAG sendtag, SWM_VC sendreqvc, SWM_VC sendrspvc, SWM_BUF sendbuf, SWM_BYTES sendbytes,
		SWM_BYTES pktrspbytes, SWM_PEER recvpeer, SWM_TAG recvtag, SWM_BUF recvbuf, SWM_ROUTING_TYPE reqrt, SWM_ROUTING_TYPE rsprt );
	void recv(SWM_PEER peer, SWM_COMM_ID comm_id, SWM_TAG tag, SWM_BUF buf, SWM_BYTES bytes);
	void irecv(SWM_PEER peer, SWM_COMM_ID comm_id, SWM_TAG tag, SWM_BUF buf, SWM_BYTES bytes, uint32_t* handle);
	void waitall(int len, uint32_t * req_ids);
	void wait(uint32_t req_ids);
	void allreduce( SWM_BYTES bytes, SWM_BYTES rspbytes, SWM_COMM_ID comm_id, SWM_VC sendreqvc, SWM_VC sendrspvc, SWM_BUF sendbuf, SWM_BUF rcvbuf,
        SWM_UNKNOWN auto1, SWM_UNKNOWN2 auto2, SWM_ROUTING_TYPE reqrt, SWM_ROUTING_TYPE rsprt);
	void barrier( SWM_COMM_ID comm_id, SWM_VC reqvc, SWM_VC rspvc, SWM_BUF buf, SWM_UNKNOWN auto1, SWM_UNKNOWN2 auto2, SWM_ROUTING_TYPE reqrt,
        SWM_ROUTING_TYPE rsprt);
	void compute(double ns);
	void finalize();

  private:

    enum SWM_type {
        FOREACH_FUNCTION(GENERATE_ENUM)
    } m_type;

    union { 
	    struct {
            SWM_PEER peer;
            SWM_COMM_ID comm_id;
            SWM_TAG tag;
            SWM_VC reqvc;
            SWM_VC rspvc;
            SWM_BUF buf;
            SWM_BYTES bytes;
            SWM_BYTES pktrspbytes;
            SWM_ROUTING_TYPE reqrt;
            SWM_ROUTING_TYPE rsprt;
            uint32_t* handle;
    	} send;
        struct { 
            SWM_PEER peer;
            SWM_COMM_ID comm_id;
            SWM_TAG tag;
            SWM_BUF buf;
            SWM_BYTES bytes;
            uint32_t* handle;
        } recv;
		struct {
         	SWM_COMM_ID comm_id;
         	SWM_PEER sendpeer;
         	SWM_TAG sendtag;
         	SWM_VC sendreqvc;
         	SWM_VC sendrspvc;
         	SWM_BUF sendbuf;
         	SWM_BYTES sendbytes;
         	SWM_BYTES pktrspbytes;
         	SWM_PEER recvpeer;
         	SWM_TAG recvtag;
         	SWM_BUF recvbuf;
         	SWM_ROUTING_TYPE reqrt;
         	SWM_ROUTING_TYPE rsprt;
		} sendrecv; 
		struct {
        	SWM_BYTES bytes;
        	SWM_BYTES rspbytes;
        	SWM_COMM_ID comm_id;
        	SWM_VC sendreqvc;
        	SWM_VC sendrspvc;
        	SWM_BUF sendbuf;
        	SWM_BUF rcvbuf;
        	SWM_UNKNOWN auto1;
        	SWM_UNKNOWN2 auto2;
        	SWM_ROUTING_TYPE reqrt;
        	SWM_ROUTING_TYPE rsprt;
		} allreduce;
		struct {
        	SWM_COMM_ID comm_id;
        	SWM_VC reqvc;
        	SWM_VC rspvc;
        	SWM_BUF buf;
        	SWM_UNKNOWN auto1;
        	SWM_UNKNOWN2 auto2;
        	SWM_ROUTING_TYPE reqrt;
        	SWM_ROUTING_TYPE rsprt;
		} barrier;
        struct {
            double ns;
        } compute;
        struct {
            int len;
            uint32_t* req_ids;
        } waitall;
        struct {
            uint32_t req_id;
        } wait;
    } m_args;

    typedef ArgStatic_Functor <Convert, int, int, bool> Functor;	

    bool handleReturn( int type, int retVal);
    bool handleSendRecvIrecvReturn( int notused, int retVal );
    bool handleSendRecvSendReturn( int notused, int retVal );
    void signalSST( SWM_type );
    void waitForSST();
    void signalWorkload();

    std::tuple<uint32_t, MessageRequest*> allocMsgReq() {
        MessageRequest* req = new MessageRequest;
        uint32_t num = m_reqNum++;

        m_msgReqMap[num] = req;
        return std::make_tuple(num,req);
    }

    MessageRequest* findMsgReq( uint32_t num ) {
        return m_msgReqMap.at(num);
    }
    void freeMsgReq( uint32_t  num ) {
        delete m_msgReqMap.at(num);
        m_msgReqMap.erase(num);
    }

    std::mutex m_mtx;
    std::condition_variable m_cv;
	std::vector<MessageResponse> m_resp;
	std::vector<MessageRequest> m_req;

	Functor initFunctor;
	Functor finiFunctor;
	Functor sendFunctor;
	Functor isendFunctor;
	Functor recvFunctor;
	Functor irecvFunctor;
	Functor sendrecvFunctor;
	Functor sendrecvIrecvFunctor;
	Functor sendrecvSendFunctor;
	Functor waitFunctor;
	Functor waitallFunctor;
	Functor allreduceFunctor;
	Functor barrierFunctor;

    Output  m_output;
    Link* m_selfLink;
	MP::Interface* m_mp;
	int m_rank;
    double m_clockFreq;
    uint32_t m_reqNum;
    std::map<uint32_t, MessageRequest* > m_msgReqMap;
};

inline void Convert::signalWorkload( ) {
    std::unique_lock<std::mutex> lck(m_mtx);
    m_output.debug( CALL_INFO, 2, 0, "thread=%" PRIx64 "\n",std::this_thread::get_id());
    m_type = Empty;
    m_cv.notify_all();
}

inline void Convert::signalSST( SWM_type type ) {
    std::unique_lock<std::mutex> lck(m_mtx);
    ConvertDBG(2, m_rank,"thread=%" PRIx64 " type=%d\n",std::this_thread::get_id(),type);
    m_type = type;
    m_cv.notify_all();
}

inline void Convert::waitForSST() {
    ConvertDBG(2, m_rank,"thread=%" PRIx64 " enter\n",std::this_thread::get_id());
    std::unique_lock<std::mutex> lck(m_mtx);
    while ( m_type != Empty ) m_cv.wait(lck);
    ConvertDBG(2, m_rank,"thread=%" PRIx64 " return\n",std::this_thread::get_id());
}

inline void Convert::init() {
    ConvertDBG(1, m_rank,"\n");
    signalSST( Init );
    waitForSST( );
}

inline void Convert::finalize() {
    ConvertDBG(1, m_rank,"\n");
    signalSST( Finalize );
    waitForSST( );
}

inline void Convert::send(SWM_PEER peer,
              SWM_COMM_ID comm_id,
              SWM_TAG tag,
              SWM_VC reqvc,
              SWM_VC rspvc,
              SWM_BUF buf,
              SWM_BYTES bytes,
              SWM_BYTES pktrspbytes,
              SWM_ROUTING_TYPE reqrt,
              SWM_ROUTING_TYPE rsprt)
{
    ConvertDBG(1, m_rank,"peer=%d comm_id=%d tag=%#x bytes=%d\n",peer,comm_id,tag,bytes);
    m_args.send.peer = peer;
    m_args.send.comm_id = comm_id;
    m_args.send.tag = tag;
    m_args.send.reqvc = reqvc;
    m_args.send.rspvc = rspvc;
    m_args.send.buf = buf;
    m_args.send.bytes = bytes;
    m_args.send.pktrspbytes = pktrspbytes;
    m_args.send.reqrt = reqrt;
    m_args.send.rsprt = rsprt;

    signalSST( Send );
    waitForSST( );
}

inline void Convert::isend(SWM_PEER peer,
              SWM_COMM_ID comm_id,
              SWM_TAG tag,
              SWM_VC reqvc,
              SWM_VC rspvc,
              SWM_BUF buf,
              SWM_BYTES bytes,
              SWM_BYTES pktrspbytes,
              uint32_t * handle,
              SWM_ROUTING_TYPE reqrt,
              SWM_ROUTING_TYPE rsprt)
{
    ConvertDBG(1, m_rank,"peer=%d comm_id=%d tag=%#x bytes=%d handle=%d\n",peer,comm_id,tag,bytes,*handle);

    m_args.send.peer = peer;
    m_args.send.comm_id = comm_id;
    m_args.send.tag = tag;
    m_args.send.reqvc = reqvc;
    m_args.send.rspvc = rspvc;
    m_args.send.buf = buf;
    m_args.send.bytes = bytes;
    m_args.send.pktrspbytes = pktrspbytes;
    m_args.send.reqrt = reqrt;
    m_args.send.rsprt = rsprt;
    m_args.send.handle = handle;

    signalSST( Isend );
    waitForSST( );
}

inline void Convert::sendrecv( SWM_COMM_ID comm_id, SWM_PEER sendpeer, SWM_TAG sendtag, SWM_VC sendreqvc, SWM_VC sendrspvc, SWM_BUF sendbuf, SWM_BYTES sendbytes,
		SWM_BYTES pktrspbytes, SWM_PEER recvpeer, SWM_TAG recvtag, SWM_BUF recvbuf, SWM_ROUTING_TYPE reqrt, SWM_ROUTING_TYPE rsprt )
{
    ConvertDBG(1, m_rank,"peer=%d comm_id=%d tag=%#x bytes=%d handle=%d\n",peer,comm_id,tag,bytes,*handle);

	m_args.sendrecv.comm_id = comm_id;
	m_args.sendrecv.sendpeer = sendpeer;
	m_args.sendrecv.sendtag = sendtag;
	m_args.sendrecv.sendreqvc = sendreqvc;
	m_args.sendrecv.sendrspvc = sendrspvc;
	m_args.sendrecv.sendbuf = sendbuf;
	m_args.sendrecv.sendbytes = sendbytes;
	m_args.sendrecv.pktrspbytes = pktrspbytes;
	m_args.sendrecv.recvpeer = recvpeer;
	m_args.sendrecv.recvtag = recvtag;
	m_args.sendrecv.reqrt = reqrt;
	m_args.sendrecv.rsprt = rsprt;

    signalSST( SendRecv );
    waitForSST( );
}

inline void Convert::recv(SWM_PEER peer,
        SWM_COMM_ID comm_id,
        SWM_TAG tag,
        SWM_BUF buf,
		SWM_BYTES bytes )
{
    ConvertDBG(1, m_rank,"peer=%d comm_id=%d tag=%#x bytes=%d\n",peer,comm_id,tag,bytes);
    m_args.recv.peer = peer;
    m_args.recv.comm_id = comm_id;
    m_args.recv.tag = tag;
    m_args.recv.buf = buf;
    m_args.recv.bytes = bytes;

    signalSST( Recv );
    waitForSST( );
}

inline void Convert::irecv(SWM_PEER peer,
            SWM_COMM_ID comm_id,
            SWM_TAG tag,
            SWM_BUF buf,
			SWM_BYTES bytes,
            uint32_t* handle)
{
    ConvertDBG(1, m_rank,"peer=%d comm_id=%d tag=%#x bytes=%d handle=%d\n",peer,comm_id,tag,bytes,*handle);
    m_args.recv.peer = peer;
    m_args.recv.comm_id = comm_id;
    m_args.recv.tag = tag;
    m_args.recv.buf = buf;
    m_args.recv.bytes = bytes;
    m_args.recv.handle = handle;

    signalSST( Irecv );
    waitForSST( );
}

inline void Convert::barrier( SWM_COMM_ID comm_id,
				SWM_VC reqvc,
				SWM_VC rspvc,
				SWM_BUF buf,
				SWM_UNKNOWN auto1,
				SWM_UNKNOWN2 auto2,
				SWM_ROUTING_TYPE reqrt,
        		SWM_ROUTING_TYPE rsprt)
{
    ConvertDBG(1, m_rank,"bytes=%d rspbytes=%d comm_id=%d sendreqvc=%d sendrspvc=%d auto1=%d auto2=%d reqrt=%d rsprt=%d\n",
			bytes,rspbytes,comm_id,sendreqvc,sendrspvc,sendbuf,rcvbuf,auto1,auto2,reqrt,rsprt);

    m_args.barrier.comm_id = comm_id;
    m_args.barrier.reqvc = reqvc;
    m_args.barrier.rspvc = rspvc;
    m_args.barrier.buf = buf;
    m_args.barrier.auto1 = auto1;
    m_args.barrier.auto2 = auto2;
    m_args.barrier.reqrt = reqrt;
    m_args.barrier.rsprt = rsprt;

    signalSST( Barrier );
    waitForSST( );
}
inline void Convert::allreduce( SWM_BYTES bytes, 
				SWM_BYTES rspbytes,
				SWM_COMM_ID comm_id,
				SWM_VC sendreqvc,
				SWM_VC sendrspvc,
				SWM_BUF sendbuf,
				SWM_BUF rcvbuf,
				SWM_UNKNOWN auto1,
				SWM_UNKNOWN2 auto2,
				SWM_ROUTING_TYPE reqrt,
				SWM_ROUTING_TYPE rsprt)
{
    ConvertDBG(1, m_rank,"bytes=%d rspbytes=%d comm_id=%d sendreqvc=%d sendrspvc=%d auto1=%d auto2=%d reqrt=%d rsprt=%d\n",
			bytes,rspbytes,comm_id,sendreqvc,sendrspvc,sendbuf,rcvbuf,auto1,auto2,reqrt,rsprt);

    m_args.allreduce.bytes = bytes;
    m_args.allreduce.rspbytes = rspbytes;
    m_args.allreduce.comm_id = comm_id;
    m_args.allreduce.sendreqvc = sendreqvc;
    m_args.allreduce.sendrspvc = sendrspvc;
    m_args.allreduce.sendbuf = sendbuf;
    m_args.allreduce.rcvbuf = rcvbuf;
    m_args.allreduce.auto1 = auto1;
    m_args.allreduce.auto2 = auto2;
    m_args.allreduce.reqrt = reqrt;
    m_args.allreduce.rsprt = rsprt;

    signalSST( Allreduce );
    waitForSST( );
}

inline void Convert::compute(double ns)
{
    ConvertDBG(1, m_rank,"time=%f ns.\n",ns);
    m_args.compute.ns = ns;

    signalSST( Compute );
    waitForSST( );
}

inline void Convert::wait( uint32_t req_id) 
{
    ConvertDBG(1, m_rank,"len=%d \n",len);
    m_args.wait.req_id = req_id;

    signalSST( Wait );
    waitForSST( );
}

inline void Convert::waitall(int len, uint32_t * req_ids) 
{
    ConvertDBG(1, m_rank,"len=%d \n",len);
    m_args.waitall.len = len;
    m_args.waitall.req_ids = req_ids;

    signalSST( Waitall );
    waitForSST( );
}

}
}
#endif
