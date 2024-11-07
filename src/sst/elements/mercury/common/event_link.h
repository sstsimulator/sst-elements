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

#include <sst/core/params.h>
#include <sst/core/event.h>
#include <mercury/common/component.h>

#define Connectable_type_invalid(ty) \
   spkt_throw_printf(sprockit::value_error, "invalid Connectable type %s", Connectable::str(ty))

#define connect_str_case(x) case x: return #x

namespace SST {
namespace Hg {

class EventLink {
 public:
  EventLink(const std::string& name, TimeDelta selflat, SST::Link* link) :
    link_(link),
    selflat_(selflat),
    name_(name)
  {
  }

  using ptr = std::unique_ptr<EventLink>;

  virtual ~EventLink() {}

  std::string toString() const;

  void send(TimeDelta delay, Event* ev);

  void send(Event* ev);

 private:
  SST::Link* link_;
  TimeDelta selflat_;
  std::string name_;
};

} // end of namespace Hg
} // end of namespace SST
