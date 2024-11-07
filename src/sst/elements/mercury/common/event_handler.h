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
