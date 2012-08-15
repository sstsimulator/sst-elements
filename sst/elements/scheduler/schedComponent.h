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

#ifndef _SCHEDCOMPONENT_H
#define _SCHEDCOMPONENT_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include "AllocInfo.h"
#include "ArrivalEvent.h"
#include "CompletionEvent.h"
#include "JobStartEvent.h"
#include "JobKillEvent.h"
#include "JobFaultEvent.h"
#include "FinalTimeEvent.h"

using namespace std;

struct IAI {int i; AllocInfo *ai;};

class schedComponent : public SST::Component {
public:

  schedComponent(SST::ComponentId_t id, SST::Component::Params_t& params);
  int Setup() {return 0;}
  int Finish();
  Machine* getMachine();
  void startJob(AllocInfo* ai);

private:
  unsigned long lastfinaltime;
  schedComponent();  // for serialization only
  schedComponent(const schedComponent&); // do not implement
  void operator=(const schedComponent&); // do not implement
  
  bool newJobLine( std::string line );

  void handleCompletionEvent( SST::Event *ev, int n );
  void handleJobArrivalEvent( SST::Event *ev );
  //virtual bool clockTic( SST::Cycle_t );

  typedef vector<int> targetList_t;

  vector<Job> jobs;
  vector<CompletionEvent*> finishingcomp;
  vector<ArrivalEvent*> finishingarr;
  Machine* machine;
  Scheduler* scheduler;
  Allocator* theAllocator;
  Statistics* stats;
  std::vector<SST::Link*> nodes;
  SST::Link* selfLink;
  map<int, IAI> runningJobs;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(nodes);
    ar & BOOST_SERIALIZATION_NVP(selfLink);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(nodes);
    ar & BOOST_SERIALIZATION_NVP(selfLink);
    //restore links
    for (unsigned int i = 0; i < nodes.size(); ++i) {
      nodes[i]->setFunctor(new SST::Event::Handler<schedComponent,int>(this,
								       &schedComponent::handleCompletionEvent,i));
    }
    
    selfLink->setFunctor(new SST::Event::Handler<schedComponent>(this,
								   &schedComponent::handleJobArrivalEvent));
  }
    
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

#endif /* _SCHEDCOMPONENT_H */
