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

#ifndef _ptlEventInternal_h
#define _ptlEventInternal_h

#include "portals4.h"

namespace SST {
namespace Portals4 {

struct PtlEventInternal  {
    volatile long  count1;
    ptl_event_t user;
    volatile long  count2;
};

}
}

#endif
