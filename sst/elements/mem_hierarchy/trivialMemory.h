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


#ifndef _SIMPLECOMPONENT_H
#define _SIMPLECOMPONENT_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include "memEvent.h"

namespace SST {
	namespace MemHierarchy {

class trivialMemory : public SST::Component {
public:

  trivialMemory(SST::ComponentId_t id, SST::Component::Params_t& params);
  int Setup() {return 0;}
  int Finish() { return 0;}

private:
  trivialMemory();  // for serialization only
  trivialMemory(const trivialMemory&); // do not implement
  void operator=(const trivialMemory&); // do not implement

  void handleRequest( SST::Event *ev );  // From CPU
  void handleSelfEvent( SST::Event *ev );

  void handleReadRequest(MemEvent *ev);
  void handleWriteRequest(MemEvent *ev);
  void sendBusPacket(void);

  void sendEvent(MemEvent *ev);
  bool canceled(MemEvent *ev);
  void cancelEvent(MemEvent* ev);

  std::deque<MemEvent*> reqs;

  SST::Link* bus_link;
  SST::Link* self_link;
  uint8_t *data;
  uint32_t memSize;
  bool bus_requested;
  /* map address to boolean:  true == req was canceled */
  std::map<Addr, bool> outstandingReqs;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(bus_link);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version)
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(bus_link);
    //resture links
    bus_link->setFunctor(new SST::Event::Handler<trivialMemory>(this,&trivialMemory::handleRequest));
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()

};

}}
#endif /* _SIMPLECOMPONENT_H */
