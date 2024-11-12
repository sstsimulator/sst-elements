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

#include <mercury/common/event_link.h>

#define connect_str_case(x) case x: return #x

namespace SST {
namespace Hg {


std::string EventLink::toString() const {
    return "self link: " + name_;
}

void EventLink::send(TimeDelta delay, Event* ev){
    //the link should have a time converter built-in?
    link_->send(SST::SimTime_t((delay + selflat_).ticks()), ev);
}

void EventLink::send(Event* ev){
    send(selflat_, ev);
}

} // end of namespace Hg
} // end of namespace SST
