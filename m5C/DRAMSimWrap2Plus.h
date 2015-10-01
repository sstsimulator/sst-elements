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

#ifndef _DRAMSimWrap2Plus_h
#define _DRAMSimWrap2Plus_h

#include <DRAMSimWrap2.h>

namespace SST {
namespace M5 {

class M5;
class MemLink;

struct DRAMSimWrap2PlusParams : public DRAMSimWrap2Params
{
    std::string linkName;
};

class MemLink;

class DRAMSimWrap2Plus : public DRAMSimWrap2
{
  public:
    DRAMSimWrap2Plus( const DRAMSimWrap2PlusParams* p );

  private:
    MemLink* m_link;
};

}
}
#endif

