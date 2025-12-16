// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

//#include <mercury/common/skeleton.h>
#include <mercury/common/timestamp.h>
#include <mercury/operating_system/libraries/unblock_event.h>
#include <mercury/components/operating_system_api.h>
#include <mercury/components/operating_system_impl.h>

namespace SST::Hg {

unsigned int
ssthg_sleep(unsigned int secs) {
    OperatingSystemAPI* cos = OperatingSystemImpl::currentOs();
    Thread* t = cos->activeThread();
    UnblockEvent* ev = new UnblockEvent(cos, t);
    cos->sendDelayedExecutionEvent(TimeDelta(secs, TimeDelta::one_second), ev);
    cos->block();
    return 0;
}

unsigned int
ssthg_usleep(unsigned int usecs) {
    OperatingSystemAPI* cos = OperatingSystemImpl::currentOs();
    Thread* t = cos->activeThread();
    UnblockEvent* ev = new UnblockEvent(cos, t);
    cos->sendDelayedExecutionEvent(TimeDelta(usecs, TimeDelta::one_microsecond), ev);
    cos->block();
    return 0;
}

unsigned int
ssthg_nanosleep(unsigned int nsecs) {
    OperatingSystemAPI* cos = OperatingSystemImpl::currentOs();
    Thread* t = cos->activeThread();
    UnblockEvent* ev = new UnblockEvent(cos, t);
    cos->sendDelayedExecutionEvent(TimeDelta(nsecs, TimeDelta::one_nanosecond), ev);
    cos->block();
    return 0;
}

} //end namespace SST::Hg

