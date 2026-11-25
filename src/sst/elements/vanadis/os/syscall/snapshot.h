// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OS_SYSCALL_SNAPSHOT
#define _H_VANADIS_OS_SYSCALL_SNAPSHOT

#include "os/syscall/syscall.h"
#include "os/callev/voscallsnapshot.h"
#include "os/vsnapshotreq.h"

namespace SST {
namespace Vanadis {

class VanadisSnapshotSyscall : public VanadisSyscall {
public:
    VanadisSnapshotSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallSnapshotEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "brk" )
    {
        printf("%s() core=%d thread=%d\n", __func__, event->getCoreID(), event->getThreadID());

        coreLink->send( new VanadisSnapshotReq( event->getCoreID(), event->getThreadID()) );

        setReturnSuccess(0);
    }
};

} // namespace Vanadis
} // namespace SST

#endif
