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

#pragma once

#include <sst/core/event.h>
#include <cstdint>
#include <tuple>

namespace SST {
namespace Hg {

template<int ...>
struct seq { };

template<int N, int ...S>
struct gens : gens<N-1, N-1, S...> { };

template<int ...S>
struct gens<0, S...> {
  typedef seq<S...> type;
};

class ExecutionEvent : public Event
  {
    NotSerializable(ExecutionEvent)
   public:
    ~ExecutionEvent() override {}

    virtual void execute() override = 0;

    ExecutionEvent() :
      linkId_(-1),
      seqnum_(-1)
    {
    }

  //  Timestamp time() const {
  //    return time_;
  //  }

  //  void setTime(const Timestamp& t) {
  //    time_ = t;
  //  }

    void setSeqnum(uint32_t seqnum) {
      seqnum_ = seqnum;
    }

    void setLink(uint32_t id){
      linkId_ = id;
    }

    uint32_t seqnum() const {
      return seqnum_;
    }

    uint32_t linkId() const {
      return linkId_;
    }

   protected:
  //  Timestamp time_;
    uint32_t linkId_;
    /** A unique sequence number from the source */
    uint32_t seqnum_;

};

template <class Cls, typename Fxn, class ...Args>
class MemberFxnCallback :
    public ExecutionEvent
{

 public:
  ~MemberFxnCallback() override{}

  void execute() override {
    dispatch(typename gens<sizeof...(Args)>::type());
  }

  MemberFxnCallback(Cls* obj, Fxn fxn, const Args&... args) :
    params_(args...),
    fxn_(fxn),
    obj_(obj)
  {
  }

 private:
  template <int ...S> void dispatch(seq<S...>){
    (obj_->*fxn_)(std::get<S>(params_)...);
  }

  std::tuple<Args...> params_;
  Fxn fxn_;
  Cls* obj_;
};

template<class Cls, typename Fxn, class ...Args>
ExecutionEvent* newCallback(Cls* cls, Fxn fxn, const Args&... args)
{
  return new MemberFxnCallback<Cls, Fxn, Args...>(cls, fxn, args...);
}

} // end of namespace Hg
} // end of namespace SST
