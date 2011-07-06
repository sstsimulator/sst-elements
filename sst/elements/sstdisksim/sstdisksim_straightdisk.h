// Copyright 2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2011, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _SSTDISKSIM_STRAIGHTDISK_H
#define _SSTDISKSIM_STRAIGHTDISK_H

#include "sstdisksim_diskmodel.h"

using namespace std;

class sstdisksim_straightdisk: public sstdisksim_diskmodel {
  sstdisksim_event* getNextEvent();
  void sstdisksim_event* addEvent(sstdisksim_event* ev);

  int Setup();
  int Finish();
  
  bool clock(Cycle_t current);

 private:
  SST::Link* link;
}
