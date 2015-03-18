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


#ifndef OFFCPUIF_H
#define OFFCPUIF_H

/// @brief Off-CPU interface (abstract) class
///
/// This class provides an interface for the simulator to use to
/// generate off-cpu events
///

namespace McNiagara{
class OffCpuIF
{
  public:
     enum access_mode {AM_READ,AM_WRITE};
     virtual ~OffCpuIF() {};
     virtual void  memoryAccess(access_mode mode, unsigned long long address, unsigned long data_size){};
     virtual void  NICAccess(access_mode mode, unsigned long data_size){};
};
}//End namespace McNiagara
#endif
