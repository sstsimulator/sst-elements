// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMMON_MEMORYMANAGER_H
#define COMMON_MEMORYMANAGER_H
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "tlm.h"

#include <sst/core/sst_types.h>

namespace SST{
class Output;
namespace SysC{
class MemoryManager : public tlm::tlm_mm_interface{
 private:
  typedef SST::SysC::MemoryManager      ThisType;
  typedef tlm::tlm_mm_interface         BaseType;
 protected:
  typedef tlm::tlm_generic_payload     Payload_t;
 public:
  MemoryManager(Output *_out):out(_out){}
  void free(Payload_t* _trans);
 private:
  Output *out;
};
}//namespace SysC
}//namespace SST

#endif//COMMON_MEMORYMANAGER_H
