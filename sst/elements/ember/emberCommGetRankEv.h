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


#ifndef _H_EMBER_GET_RANK_EV
#define _H_EMBER_GET_RANK_EV

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberCommGetRankEvent : public EmberEvent {
	public:
		EmberCommGetRankEvent(Communicator comm, uint32_t* rankPtr )
            : m_comm(comm), m_rankPtr(rankPtr) {}
		~EmberCommGetRankEvent() {} 
		EmberEventType getEventType() { return COMM_GET_RANK; }
        std::string getPrintableString() { 
            return "GET_RANK Event";
        }
        Communicator getComm() { return m_comm; } 
        uint32_t*    getRankPtr() { return m_rankPtr; }

	private:
		Communicator m_comm;
        uint32_t*    m_rankPtr;
};

}
}

#endif
