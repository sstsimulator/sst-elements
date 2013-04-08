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

#ifndef _SSTDISKSIM_RAID0_H
#define _SSTDISKSIM_RAID0_H

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

struct raid0_link_list
{
  SST::Link* link;
  struct raid0_link_list* next;
}raid0_link_list;

class sstdisksim_raid0 : public sstdisksim_diskmodel, public Component {
 public:

  sstdisksim_raid0( ComponentId_t id, Params_t& params );
  ~sstdisksim_raid0();

  int Setup();
  int Finish();

  bool clock(Cycle_t current);

 private:

  Params_t __params;
  ComponentId_t __id;

  sstdisksim_event* getNextEvent();

  void handleEvent(Event* ev);

  int num_devices;
  int cur_device;

  Log< DISKSIM_DBG >&  __dbg;
  
  SST::Link* raid0;

  // Want to remove this so we can have a variable number of disk eventually.
  // Tentative logic for a replacement is below in the next comment.
  SST::Link* disk0;
  SST::Link* disk1;
  //  struct raid0_link_list* head_link;
  
  friend class boost::serialization::access;
  template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
      ar & BOOST_SERIALIZATION_NVP(link);
      ar & BOOST_SERIALIZATION_NVP(raid0);
    }
  
  template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
      //      ar & BOOST_SERIALIZATION_NVP(head_link);
      ar & BOOST_SERIALIZATION_NVP(raid0);
      ar & BOOST_SERIALIZATION_NVP(disk0);
      ar & BOOST_SERIALIZATION_NVP(disk1);
      ar & BOOST_SERIALIZATION_NVP(num_devices);
      ar & BOOST_SERIALIZATION_NVP(cur_device);

      SST::Link* raid0;
      
      raid0->setFunctor(new 
			SST::Event::Handler<sstdisksim_raid0>(this, 
							      &sstdisksim_raid0::handleEvent));
    }
	
  BOOST_SERIALIZATION_SPLIT_MEMBER()    
};

#endif /* _SSTDISKSIM_RAID0_H */
