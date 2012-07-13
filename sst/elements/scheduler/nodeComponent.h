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

#ifndef _NODECOMPONENT_H
#define _NODECOMPONENT_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include "JobStartEvent.h"
#include "CompletionEvent.h"
#include "JobFaultEvent.h"

#include <vector>

class nodeComponent : public SST::Component {
public:

  nodeComponent(SST::ComponentId_t id, SST::Component::Params_t& params);
  int Setup() {return 0;}
  int Finish() {return 0;}

private:
  nodeComponent();  // for serialization only
  nodeComponent(const nodeComponent&); // do not implement
  void operator=(const nodeComponent&); // do not implement

  void handleEvent( SST::Event *ev );
  void handleSelfEvent( SST::Event *ev );
  
  void sendNextFault( std::string faultType );

  int jobNum;
  int nodeNum;
  float lambda;

  SST::Link* Scheduler;
  SST::Link* SelfLink;

  std::vector<SST::Link *> Parents;
  std::vector<SST::Link *> Children;
  
  std::map<std::string, float> Faults;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(jobNum);
    ar & BOOST_SERIALIZATION_NVP(nodeNum);
    ar & BOOST_SERIALIZATION_NVP(Scheduler);
    ar & BOOST_SERIALIZATION_NVP(SelfLink);

    ar & BOOST_SERIALIZATION_NVP(Parents);
    ar & BOOST_SERIALIZATION_NVP(Children);
    
    ar & BOOST_SERIALIZATION_NVP(Faults);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(jobNum);
    ar & BOOST_SERIALIZATION_NVP(nodeNum);
    ar & BOOST_SERIALIZATION_NVP(Scheduler);
    ar & BOOST_SERIALIZATION_NVP(SelfLink);

    ar & BOOST_SERIALIZATION_NVP(Parents);
    ar & BOOST_SERIALIZATION_NVP(Children);
    
    ar & BOOST_SERIALIZATION_NVP(Faults);

    //restore links
    Scheduler->setFunctor(new SST::Event::Handler<nodeComponent>(this,
								 &nodeComponent::handleEvent));
    SelfLink->setFunctor(new SST::Event::Handler<nodeComponent>(this,
								&nodeComponent::handleSelfEvent));
	  
	  for( unsigned int counter = 0; counter < Parents.size(); counter ++ ){
	    Parents.at( counter )->setFunctor( new SST::Event::Handler<nodeComponent>( this,
							                                                                   &nodeComponent::handleEvent ) );
	  }
		
		
	  for( unsigned int counter = 0; counter < Children.size(); counter ++ ){
	    Children.at( counter )->setFunctor( new SST::Event::Handler<nodeComponent>( this,
							                                                                    &nodeComponent::handleEvent ) );
	  }
  }
    
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

#endif /* _NODECOMPONENT_H */
