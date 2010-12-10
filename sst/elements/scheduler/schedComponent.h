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
#include "SWFEvent.h"

using namespace std;

class schedComponent : public SST::Component {
public:

  schedComponent(SST::ComponentId_t id, SST::Component::Params_t& params);
  int Setup() {return 0;}
  int Finish() {return 0;}

private:
  schedComponent();  // for serialization only
  schedComponent(const schedComponent&); // do not implement
  void operator=(const schedComponent&); // do not implement

  void handleEvent( SST::Event *ev, int n );
  virtual bool clockTic( SST::Cycle_t );

  typedef vector<int> targetList_t;
  void startJob(SWFEvent *e, targetList_t targets);

  std::vector<SST::Link*> nodes;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(nodes);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(nodes);
    //restore links
    for (unsigned int i = 0; i < nodes.size(); ++i) {
      nodes[i]->setFunctor(new SST::Event::Handler<schedComponent,int>(this,
								       &schedComponent::handleEvent,i));
    }
  }
    
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

#endif /* _SCHEDCOMPONENT_H */
