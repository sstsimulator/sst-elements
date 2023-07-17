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

#include <mercury/common/node_address.h>
#include <mercury/common/events.h>
#include <mercury/common/printable.h>
#include <tuple>

#include <sst/core/link.h>

namespace SST::Hg {

/**
 * The main interface for something that can respond to an event (sst_message).
 */
class EventHandler : public SST::Hg::printable
{
 public:
  virtual ~EventHandler() {}

  virtual void handle(Event* ev) = 0;

 protected:
  EventHandler() {}

};

//template<int ...>
//struct seq { };

//template<int N, int ...S>
//struct gens : gens<N-1, N-1, S...> { };

//template<int ...S>
//struct gens<0, S...> {
//  typedef seq<S...> type;
//};

template<typename C>
struct has_deadlock_check {
private:
    template<typename T>
    static constexpr auto check(T*)
    -> typename
        std::is_same<
            decltype( std::declval<T>().deadlock_check() ),
            void
        >::type;  // attempt to call it and see if the return type is correct

    template<typename>
    static constexpr std::false_type check(...);

    typedef decltype(check<C>(0)) type;

public:
    static constexpr bool value = type::value;
};

template <class Cls, typename Fxn, class ...Args>
class MemberFxnHandler : public EventHandler
{

 public:
  ~MemberFxnHandler() override{}

  std::string toString() const override {
    return obj_->toString();
  }

  void handle(Event* ev) override {
    dispatch(ev, typename gens<sizeof...(Args)>::type());
  }

  MemberFxnHandler(Cls* obj, Fxn fxn, const Args&... args) :
    params_(args...),
    fxn_(fxn),
    obj_(obj)
  {
  }

 private:
  template <int ...S>
  void dispatch(Event* ev, seq<S...>){
    (obj_->*fxn_)(ev, std::get<S>(params_)...);
  }

  template <class T = Cls>
  typename std::enable_if<has_deadlock_check<T>::value>::type
  localDeadlockCheck() {
    obj_->deadlock_check();
  }

  template <class T = Cls>
  typename std::enable_if<has_deadlock_check<T>::value>::type
  localDeadlockCheck(Event* ev) {
    obj_->deadlock_check(ev);
  }

  template <class T = Cls>
  typename std::enable_if<!has_deadlock_check<T>::value>::type
  localDeadlockCheck() {}

  template <class T = Cls>
  typename std::enable_if<!has_deadlock_check<T>::value>::type
  localDeadlockCheck(Event*) {}

  std::tuple<Args...> params_;
  Fxn fxn_;
  Cls* obj_;

};

template<class Cls, typename Fxn, class ...Args>
EventHandler* newHandler(Cls* cls, Fxn fxn, const Args&... args)
{
  return new MemberFxnHandler<Cls, Fxn, Args...>(
        cls, fxn, args...);
}

} // end of namespace SST::Hg
