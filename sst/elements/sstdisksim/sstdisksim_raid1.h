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

#ifndef _SSTDISKSIM_RAID1_H
#define _SSTDISKSIM_RAID1_H

#include "sstdisksim_event.h"

#include <sst/core/log.h>
#include <sst/core/component.h>
#include <sst/core/simulation.h>
#include <stdlib.h>
#include <stddef.h>

#include "syssim_driver.h"
#include <disksim_interface.h>
#include <disksim_rand48.h>
 
#include <sstdisksim.h>
#include <sstdisksim_event.h>
#include "sstdisksim_tau_parser.h"

#include "sstdisksim_diskmodel.h"

using namespace std;
using namespace SST;

#ifndef DISKSIM_DBG
#define DISKSIM_DBG 0
#endif

struct raid1_link_list
{
  SST::Link* link;
  struct raid1_link_list* next;
}raid1_link_list;

class sstdisksim_raid1 : public sstdisksim_diskmodel, public Component {
 public:

  sstdisksim_raid1( ComponentId_t id, Params_t& params );
  ~sstdisksim_raid1();

  void setup();  
  void finish();

  bool clock(Cycle_t current);

 private:

  Params_t __params;
  ComponentId_t __id;

  sstdisksim_event* getNextEvent();

  void handleEvent(Event* ev);

  Log< DISKSIM_DBG >&  __dbg;
  
  SST::Link* raid1;

  SST::Link* disk0;
  SST::Link* disk1;
  
  friend class boost::serialization::access;
  template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
      ar & BOOST_SERIALIZATION_NVP(link);
      ar & BOOST_SERIALIZATION_NVP(raid1);
    }
  
  template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
      ar & BOOST_SERIALIZATION_NVP(raid1);
      ar & BOOST_SERIALIZATION_NVP(disk0);
      ar & BOOST_SERIALIZATION_NVP(disk1);

      SST::Link* raid1;
      
      raid1->setFunctor(new 
			SST::Event::Handler<sstdisksim_raid1>(this, 
							      &sstdisksim_raid1::handleEvent));
    }
	
  BOOST_SERIALIZATION_SPLIT_MEMBER()    
};

#endif /* _SSTDISKSIM_RAID1_H */
