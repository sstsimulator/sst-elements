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

#include <stdio.h>
#include "sstdisksim_event.h"

class SST_DiskModel {
public:
  virtual void read;
  virtual void write;
  virtual sstdisksim_event* getNextEvent();
  virtual void sstdisksim_event* addEvent(sstdisksim_event* ev);
  
 private:
  sstdisksim_event_entries __list;
}
