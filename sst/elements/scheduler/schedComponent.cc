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

#include "schedComponent.h"

using namespace SST;
using namespace std;

schedComponent::schedComponent(ComponentId_t id, Params_t& params) :
  Component(id) {

  // tell the simulator not to end without us
  registerExit();

  // configure links
  bool done = 0;
  int count = 0;
  // connect links till we can't find any
  printf("Scheduler looking for link...");
  while (!done) {
    char name[50];
    snprintf(name, 50, "nodeLink%d", count);
    printf(" %s", name);
    SST::Link *l = configureLink( name, 
				  new Event::Handler<schedComponent,int>(this,
								     &schedComponent::
									 handleEvent, count) );
    if (l) {
      nodes.push_back(l);
      count++;
    } else {
      printf("(no %s)", name);
      done = 1;
    }
  }
  
  printf("\nScheduler Detects %d nodes\n", nodes.size());

  //set our clock
  registerClock( "1 s", 
		 new Clock::Handler<schedComponent>(this, 
						     &schedComponent::clockTic ) );
}

schedComponent::schedComponent() :
    Component(-1)
{
    // for serialization only
}

// incoming events are scanned and deleted. ev is the returned event,
// node is the node it came from.
void schedComponent::handleEvent(Event *ev, int node) {
  SWFEvent *event = dynamic_cast<SWFEvent*>(ev);
  if (event) {
    if (event->jobStatus == SWFEvent::COMPLETED) {
      printf("S: Job %d completed from n%d at time %lld\n", event->JobNumber, 
	     node, getCurrentSimTime());
      delete event;
    } else {
      printf("S: Job %d had odd jobStatus %d\n", event->JobNumber, 
	     event->jobStatus);
    }
  } else {
    printf("S: Error! Bad Event Type!\n");
  }
}

void schedComponent::startJob(SWFEvent *e, targetList_t targs) {
  // send to eachperson in the node list
  for (targetList_t::iterator i = targs.begin(); i != targs.end(); ++i) {
    // make a copy of the event
    SWFEvent *ec = new SWFEvent;
    *ec = *e;
    // send it
    nodes[*i]->Send(ec);
  }

  // delete the original event
  delete e;
}

bool schedComponent::clockTic( Cycle_t ) {

  // send test jobs
  if (getCurrentSimTime() == 1) {
    // send to node 0
    // create the event
    SWFEvent *e = new SWFEvent;
    e->JobNumber = 1;
    e->RunTime = 10;
    // create the list of target nodes
    targetList_t nodeList;
    nodeList.push_back(0);
    // start the job
    startJob(e, nodeList);
  } else if (getCurrentSimTime() == 12) {
    printf("\n");
  } else if (getCurrentSimTime() == 13) {
    // send to odd nodes
    // create the event
    SWFEvent *e = new SWFEvent;
    e->JobNumber = 1;
    e->RunTime = 5;
    // create the list of target nodes
    targetList_t nodeList;
    for (int i = 0; i < nodes.size(); ++i) {
      if (i & 0x1) {
	nodeList.push_back(i);
      }
    }
    // start the job
    startJob(e, nodeList);
  }

  // end simulation after 1 hour of simulated time
  if (getCurrentSimTime() >= 3600) {
    unregisterExit();
  }

  // return false so we keep going
  return false;
}

// Element Libarary / Serialization stuff

BOOST_CLASS_EXPORT(schedComponent)

