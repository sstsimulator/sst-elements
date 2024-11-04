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

#include <sst/core/event.h>
#include <mercury/common/timestamp.h>
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

    Timestamp time() const {
      return time_;
    }

    void setTime(const Timestamp& t) {
      time_ = t;
    }

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
    Timestamp time_;
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
