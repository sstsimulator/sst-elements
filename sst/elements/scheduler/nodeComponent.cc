// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "nodeComponent.h"

using namespace SST;

nodeComponent::nodeComponent(ComponentId_t id, Params_t& params) :
  Component(id), currentJob(NULL) {

  if ( params.find("nodeNum") == params.end() ) {
    _abort(event_test,"couldn't find name for this node\n");
  }
  nodeNum = strtol( params[ "nodeNum" ].c_str(), NULL, 0 );


  Scheduler = configureLink( "Scheduler", 
		     new Event::Handler<nodeComponent>(this,
							 &nodeComponent::
							 handleEvent) );
  SelfLink = configureSelfLink("linkToSelf", 
			       new Event::Handler<nodeComponent>(this, 
								 &nodeComponent::
								 handleSelfEvent));
  SelfLink->setDefaultTimeBase(registerTimeBase("1 s"));

  assert(Scheduler);
  assert(SelfLink);

  //set our clock
  setDefaultTimeBase(registerTimeBase("1 s"));
}

nodeComponent::nodeComponent() :
    Component(-1)
{
    // for serialization only
}

// incoming events start a new job
void nodeComponent::handleEvent(Event *ev) {
  //printf("recv\n");
  SWFEvent *event = dynamic_cast<SWFEvent*>(ev);
  if (event) {
    if (currentJob == NULL) {
      // "start" the job
      currentJob = event;
      currentJob->jobStatus = SWFEvent::RUNNING;
      // set a reminder to finish the job
      printf("n%d@%llds: Launched Job %d for %d seconds\n", nodeNum, 
	     getCurrentSimTime(),
	     currentJob->JobNumber, 
	     currentJob->RunTime);
      SelfLink->Send(currentJob->RunTime, currentJob);      
    } else {
      printf("Error?! Already running a job, but given a new one!\n");
    }
  } else {
    printf("Error! Bad Event Type!\n");
  }
}

// finish current job
void nodeComponent::handleSelfEvent(Event *ev) {
  SWFEvent *event = dynamic_cast<SWFEvent*>(ev);
  if (event) {
    if (event->JobNumber == currentJob->JobNumber) {
      // "finish" the job
      currentJob->jobStatus = SWFEvent::COMPLETED;
      Scheduler->Send(event);
      currentJob = NULL;
      printf("n%d@%llds: Finished Job %d\n", 
	     nodeNum, getCurrentSimTime(), event->JobNumber);
    } else {
      printf("Error!! We are not running this job we're supposed to finish!\n");
    }
  } else {
    printf("Error! Bad Event Type!\n");
  }
}


// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(SWFEvent)
BOOST_CLASS_EXPORT(nodeComponent)

