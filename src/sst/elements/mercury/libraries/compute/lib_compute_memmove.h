// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#pragma once

#include <mercury/libraries/compute/lib_compute_inst.h>

namespace SST {
namespace Hg {

class LibComputeMemmove :
  public LibComputeInst
{

 public:
  ~LibComputeMemmove() override {}

  LibComputeMemmove(SST::Params& params, SoftwareId id,
                      OperatingSystem* os);

  LibComputeMemmove(SST::Params& params, const char* prefix, SoftwareId id,
                      OperatingSystem* os);

  void incomingEvent(Event *ev) override{
    //forward to parent, which throws
    Library::incomingEvent(ev);
  }

  void read(uint64_t bytes);

  void write(uint64_t bytes);

  void copy(uint64_t bytes);

 protected:
  static const long unlimited_page_size = -1;
  static const long default_page_size = unlimited_page_size;

  void doAccess(uint64_t bytes);

 protected:
  int access_width_bytes_;

};

}
} //end of namespace
