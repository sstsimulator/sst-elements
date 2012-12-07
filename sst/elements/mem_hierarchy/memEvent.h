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
#include "memTypes.h"

namespace SST {
namespace MemHierarchy {

class MemEvent : public SST::Event {
public:
	typedef std::vector<uint8_t> dataVec;
	typedef std::pair<uint64_t, int> id_type;

	MemEvent() {} // For serialization only
	MemEvent(Addr _addr, Command _cmd) :
		SST::Event(), addr(_addr), cmd(_cmd)
	{
		int rank; // TODO:  Determine rank another way;
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		event_id = std::make_pair(main_id++, rank);
		response_to_id = std::make_pair(0, -1);
		src = dst = 0;
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



	MemEvent* makeResponse(void)
	{
		MemEvent *me = new MemEvent(addr, commandResponse(cmd));
		me->setSize(size);
		me->response_to_id = event_id;
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
	int getSrc(void) const { return src; }
	void setSrc(int s) { src = s; }
	int getDst(void) const { return dst; }
	void setDst(int d) { dst = d; }

private:
	static uint64_t main_id;
	id_type event_id;
	id_type response_to_id;
	Addr addr;
	uint32_t size;
	Command cmd;
	dataVec payload;
	int src;
	int dst;

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
