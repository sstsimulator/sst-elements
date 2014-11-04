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


#ifndef _H_EMBER_COMM_SPLIT_EV
#define _H_EMBER_COMM_SPLIT_EV

#include <sst/elements/hermes/msgapi.h>
#include "emberevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Ember {

class EmberCommSplitEvent : public EmberEvent {
	public:
		EmberCommSplitEvent(Communicator oldComm, int color, 
                                    int key, Communicator* newComm ) 
          : m_oldComm( oldComm), m_key(key), m_color(color), m_newComm(newComm)
        {}
		~EmberCommSplitEvent() {}
		EmberEventType getEventType() { return COMM_SPLIT;} 
        std::string getPrintableString() { return "CommSplit Event"; }
		Communicator getOldComm()  { return m_oldComm; }
        Communicator* getNewComm() { return m_newComm; }
        int getKey()   { return m_key; }
        int getColor() { return m_color; }

	private:
		Communicator m_oldComm;
        int m_key;
        int m_color;
		Communicator* m_newComm;
};

}
}

#endif
