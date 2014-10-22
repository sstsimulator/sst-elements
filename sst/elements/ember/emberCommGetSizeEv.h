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


#ifndef _H_EMBER_GET_SIZE_EV
#define _H_EMBER_GET_SIZE_EV

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberCommGetSizeEvent : public EmberEvent {
	public:
		EmberCommGetSizeEvent(Communicator comm, uint32_t* sizePtr )
            : m_comm(comm), m_sizePtr(sizePtr) {}
		~EmberCommGetSizeEvent() {} 
		EmberEventType getEventType() { return COMM_GET_SIZE; }
        std::string getPrintableString() { 
            return "GET_SIZE Event";
        }
        Communicator getComm() { return m_comm; }
        uint32_t*    getSizePtr() { return m_sizePtr; }


	private:
		Communicator m_comm;
        uint32_t*    m_sizePtr;
};

}
}

#endif
