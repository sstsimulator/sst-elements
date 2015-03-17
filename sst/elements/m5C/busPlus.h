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

#ifndef _busPlus_h
#define _busPlus_h

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <mem/bus.hh>

namespace SST {
namespace M5 {

class M5;
class MemLink;

struct BusPlusParams : public BusParams
{
    M5*         m5Comp;
    SST::Params params;
};

class MemLink;

class BusPlus : public Bus
{
  public:
    BusPlus( const BusPlusParams* p );

  private:
    std::deque<MemLink*> m_links;
};

}
}

#endif
