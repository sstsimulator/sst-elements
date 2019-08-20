// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_ARIEL_NOTIFY_EVENT
#define _H_SST_ARIEL_NOTIFY_EVENT

#include <sst/core/event.h>

namespace SST {
namespace ArielComponent {

class NotifyEvent : public Event {
  ImplementSerializable(NotifyEvent)
 public:
  NotifyEvent(int core) :
    core_(core)
  {
  }

  void serialize_order(Core::Serialization::serializer &ser) override {
    ser & core_;
  }
  
  int core() const {
    return core_;
  }

 private:
  NotifyEvent(){} //for serialization

  int core_;
};

}
}

#endif
