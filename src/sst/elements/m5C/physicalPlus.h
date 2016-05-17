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

#ifndef _physicalPlus_h
#define _physicalPlus_h

#include <sst/core/component.h>
#include <mem/physical.hh>

namespace SST {
namespace M5 {

class M5;
class MemLink;

struct PhysicalMemoryPlusParams : public PhysicalMemoryParams
{
    M5*         m5Comp;
    std::string linkName;
};

class MemLink;

class PhysicalMemoryPlus : public PhysicalMemory 
{
  public:
    PhysicalMemoryPlus( const PhysicalMemoryPlusParams* p );

  private:
    MemLink* m_link;
};

}
}

#endif

