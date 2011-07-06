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

#ifndef _SSTDISKSIM_DISKMODEL_H
#define _SSTDISKSIM_DISKMODEL_H

#include <stdio.h>
#include "sstdisksim_event.h"
#include "sstdisksim_posix_call.h"

using namespace std;

class sstdisksim_diskmodel : public Component {
public:
  virtual void read();
  virtual void write();
  virtual sstdisksim_event* getNextEvent();
  virtual void sstdisksim_event* addEvent(sstdisksim_posix_call call);
  
 protected:
  sstdisksim_event_entries __list;
}

#endif // _SSTDISKSIM_DISKMODEL_H
