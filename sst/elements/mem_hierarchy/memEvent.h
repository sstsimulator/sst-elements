// Copyright 2012 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2012, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMEVENT_H
#define _MEMEVENT_H

#include <mpi.h>  // TODO:  Determine rank another way;
#include <sst/core/sst_types.h>
#include <sst/core/debug.h>
#include <sst/core/activity.h>

namespace SST {
namespace MemHierarchy {


typedef uint64_t Addr;

#define X_TYPES \
	X(ReadReq, 0) \
	X(ReadResp, 1) \
	X(WriteReq, 2) \
	X(WriteResp, 3) \
	X(Invalidate, 4) \
	X(Fetch, 5) \
	X(Fetch_Invalidate, 6) \
	X(FetchResp, 7) \
	X(WriteBack, 8) \
	X(RequestBus, 9) \
	X(CancelBusRequest, 10) \
	X(BusClearToSend, 11) \
	X(ACK, 12) \
	X(NACK, 13) \
	X(NULLCMD, 14)

typedef enum {
#define X(x, n) x = n,
	X_TYPES
#undef X
} Command;


//extern const char* CommandString[];
static const char* CommandString[] __attribute__((unused)) = {
#define X(x, n) #x ,
	X_TYPES
#undef X
};


static const std::string BROADCAST_TARGET = "BROADCAST";

class MemEvent : public SST::Event {
public:
	typedef std::vector<uint8_t> dataVec;
	typedef std::pair<uint64_t, int> id_type;

	MemEvent() {} // For serialization only
	MemEvent(const std::string &_src, Addr _addr, Command _cmd) :
		SST::Event(), addr(_addr), cmd(_cmd), src(_src)
	{
		int rank; // TODO:  Determine rank another way;
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		event_id = std::make_pair(main_id++, rank);
		response_to_id = std::make_pair(0, -1);
		dst = BROADCAST_TARGET;
		size = 0;
	}

	MemEvent(const MemEvent &me) : SST::Event()
	{
		event_id = me.event_id;
		response_to_id = me.response_to_id;
		addr = me.addr;
		size = me.size;
		cmd = me.cmd;
		payload = me.payload;
		src = me.src;
		dst = me.dst;
	}
	MemEvent(const MemEvent *me) : SST::Event()
	{
		event_id = me->event_id;
		response_to_id = me->response_to_id;
		addr = me->addr;
		size = me->size;
		cmd = me->cmd;
		payload = me->payload;
		src = me->src;
		dst = me->dst;
	}



	MemEvent* makeResponse(const std::string &source)
	{
		MemEvent *me = new MemEvent(source, addr, commandResponse(cmd));
		me->setSize(size);
		me->response_to_id = event_id;
		me->dst = src;
		return me;
	}

	id_type getID(void) const { return event_id; }
	id_type getResponseToID(void) const { return response_to_id; }
	Command getCmd(void) const { return cmd; }
	void setCmd(Command newcmd) { cmd = newcmd; }
	Addr getAddr(void) const { return addr; }
	uint32_t getSize(void) const { return size; }
	void setSize(uint32_t sz) {
		size = sz;
		payload.resize(size);
	}
	dataVec& getPayload(void) { return payload; }
	void setPayload(uint32_t size, uint8_t *data);
	void setPayload(std::vector<uint8_t> &data);
	const std::string& getSrc(void) const { return src; }
	void setSrc(const std::string &s) { src = s; }
	const std::string& getDst(void) const { return dst; }
	bool isBroadcast(void) const { return (dst == BROADCAST_TARGET); }
	void setDst(const std::string &d) { dst = d; }

private:
	static uint64_t main_id;
	id_type event_id;
	id_type response_to_id;
	Addr addr;
	uint32_t size;
	Command cmd;
	dataVec payload;
	std::string src;
	std::string dst;

	Command commandResponse(Command c)
	{
		switch(c) {
		case ReadReq:
			return ReadResp;
		case WriteReq:
			return WriteResp;
		case Fetch:
		case Fetch_Invalidate:
			return FetchResp;
		default:
			return NULLCMD;
		}
	}

	friend class boost::serialization::access;
	template<class Archive>
	void
	serialize(Archive & ar, const unsigned int version )
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
		ar & BOOST_SERIALIZATION_NVP(event_id);
		ar & BOOST_SERIALIZATION_NVP(response_to_id);
		ar & BOOST_SERIALIZATION_NVP(addr);
		ar & BOOST_SERIALIZATION_NVP(cmd);
		ar & BOOST_SERIALIZATION_NVP(payload);
		ar & BOOST_SERIALIZATION_NVP(src);
		ar & BOOST_SERIALIZATION_NVP(dst);
	}
};

}
}

#endif /* _MEMEVENT_H */
