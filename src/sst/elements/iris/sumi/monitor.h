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

#pragma once

#include <output.h>
#include <sst/core/eli/elementbuilder.h>
//#include <sprockit/debug.h>
#include <sst/core/params.h>
#include <sumi/message.h>
#include <sumi/timeout.h>
#include <sumi/transport_fwd.h>

//DeclareDebugSlot(sumi_ping)
//DeclareDebugSlot(sumi_failure)

namespace SST::Iris::sumi {

class FunctionSet {
 public:
  int erase(TimeoutFunction* func);

  void append(TimeoutFunction* func){
    listeners_.push_back(func);
  }

  int refcount() const {
    return listeners_.size();
  }

  bool empty() const {
    return listeners_.empty();
  }

  void timeoutAllListeners(int dst);

  Output output;

 protected:
  std::list<TimeoutFunction*> listeners_;
};

class ActivityMonitor
{
 public:
  ActivityMonitor(SST::Params&  /*params*/,
                  Transport* t) : api_(t){}

  virtual ~ActivityMonitor(){}

  virtual void ping(int dst, TimeoutFunction* func) = 0;

  virtual void cancelPing(int dst, TimeoutFunction* func) = 0;

  virtual void messageReceived(Message* msg) = 0;

  virtual void renewPings(double wtime) = 0;

  virtual void validateDone() = 0;

  virtual void validateAllPings() = 0;

 protected:
  Transport* api_;

};

}
