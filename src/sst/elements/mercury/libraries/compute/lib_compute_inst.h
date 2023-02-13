/**
Copyright 2009-2021 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2021, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#ifndef SSTMAC_SOFTWARE_LIBRARIES_COMPUTE_LIB_COMPUTE_INST_H_INCLUDED
#define SSTMAC_SOFTWARE_LIBRARIES_COMPUTE_LIB_COMPUTE_INST_H_INCLUDED

#include <sstmac/software/libraries/compute/lib_compute_time.h>
#include <sstmac/software/libraries/compute/compute_event_fwd.h>
#include <sstmac/software/process/software_id.h>
#include <sstmac/common/sstmac_config.h>
#include <stdint.h>
#include <sstmac/common/stats/ftq_tag.h>

//these are the default instruction labels

DeclareDebugSlot(lib_compute_inst);

namespace sstmac {
namespace sw {

class LibComputeInst :
  public LibComputeTime
{
 public:
  LibComputeInst(SST::Params& params, SoftwareId id, OperatingSystem* os);

  LibComputeInst(SST::Params& params, const std::string& libname,
                   SoftwareId id, OperatingSystem* os);

  ~LibComputeInst() override { }

  void computeInst(ComputeEvent* msg, int nthr = 1);

  void computeDetailed(uint64_t flops,
    uint64_t nintops,
    uint64_t bytes,
    int nthread = 1);

  void computeLoop(uint64_t nloops,
    uint32_t flops_per_loop,
    uint32_t intops_per_loop,
    uint32_t bytes_per_loop);

  void incomingEvent(Event *ev) override {
    Library::incomingEvent(ev);
  }

 protected:
  double loop_overhead_;

 private:
  void init(SST::Params& params);

};

}
} //end of namespace sstmac

#endif
