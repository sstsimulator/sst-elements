// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Used for communication between components, including passing instructions, requesting information, and communicating state changes.
 */

#ifndef __COMMUNICATIONEVENT_H__
#define __COMMUNICATIONEVENT_H__

#include <string>

namespace SST {
namespace Scheduler {

enum CommunicationTypes { RETRIEVE_ID, START_FAULTING, LOG_JOB_START, UNREGISTER_YOURSELF, START_FILE_WATCH, START_NEXT_JOB };

class CommunicationEvent : public SST::Event{
  public:

    CommunicationEvent( enum CommunicationTypes type ) : SST::Event(){
      CommType = type;
      reply = false;
    }

    CommunicationEvent( enum CommunicationTypes type, void * payload ) : SST::Event(){
      CommType = type;
      reply = false;
      this->payload = payload;
    }

    enum CommunicationTypes CommType;
    bool reply;
    void * payload;
};

}
}
#endif

