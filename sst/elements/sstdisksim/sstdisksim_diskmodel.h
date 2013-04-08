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

#ifndef _SSTDISKSIM_DISKMODEL_H
#define _SSTDISKSIM_DISKMODEL_H

#include "syssim_driver.h"
#include "sstdisksim_event.h"

#include <sst/core/log.h>
#include <sst/core/component.h>
#include <sst/core/simulation.h>
#include <stdlib.h>
#include <stddef.h>
#include <sst/core/timeConverter.h>

#include "sstdisksim_posix_call.h"

using namespace std;
using namespace SST;

class sstdisksim_diskmodel {
public:
  void addEvent(sstdisksim_posix_call call);
  
 protected:
  sstdisksim_posix_calls __list;
};

#endif // _SSTDISKSIM_DISKMODEL_H
