/**
Copyright 2009-2023 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2023, NTESS

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

#include <sumi/monitor.h>
#include <sumi/transport.h>

//RegisterDebugSlot(sumi_ping,
//  "print all debug output associated with ping/timeout operations in the sumi framework");
//RegisterDebugSlot(sumi_failure,
//  "print all the debug output associated with all node failures detected by the sumi framework");

namespace SST::Iris::sumi {

void
FunctionSet::timeoutAllListeners(int dst)
{
  //we have failed! loop through the functions and time them out
  std::list<TimeoutFunction*> tmp = listeners_;
  std::list<TimeoutFunction*>::iterator it, end = tmp.end();
  int idx = 0;
  for (it=tmp.begin(); it != end; ++it, ++idx){
    TimeoutFunction* func = *it;
    output.output("\ttiming out ping %p to %d", func, dst);
    func->time_out(dst);
  }
}

int
FunctionSet::erase(TimeoutFunction* func)
{
  output.output("\terasing ping %p", func);

  std::list<TimeoutFunction*>::iterator tmp,
    it = listeners_.begin(),
    end = listeners_.end();
  bool found = false;
  while(it != end){
    tmp = it++;
    TimeoutFunction* test = *tmp;
    if (test == func){
      listeners_.erase(tmp);
      found = true;
      break;
    }
  }

  if (!found){
    sst_hg_throw_printf(SST::Hg::IllformedError,
        "pinger::cancel: unknown pinger %p",
        func);
  }

  return listeners_.size();
}

}
