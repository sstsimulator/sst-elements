// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _physical2_h
#define _physical2_h

#include <mem/physical.hh>

namespace SST {
namespace M5 {

class PhysicalMemory2 : public PhysicalMemory 
{
  public:
    PhysicalMemory2( const Params* p ) : PhysicalMemory( p ) {} 
    // PhyscialMemory panics in init() if it is not connected
    void init() {}
};

}
}

#endif
