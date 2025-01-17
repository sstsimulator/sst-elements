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

#include <sst_config.h>

#include "os/syscall/mprotect.h"
#include "os/vnodeos.h"

using namespace SST::Vanadis;

VanadisMprotectSyscall::VanadisMprotectSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallMprotectEvent* event)
    : VanadisSyscall( os, coreLink, process, event, "mprotect" )
{
    m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-mprotect] core %d thread %d process %d addr=%" PRIx64 " len=%" PRIu64 " prot=%#" PRIx64 "\n",
            event->getCoreID(), event->getThreadID(), process->getpid(), event->getAddr(), event->getLen(), event->getProt() );

    int perms = 0;
    if ( event->getProt() & PROT_READ ) {
        perms |= 0x4;
    }
    if ( event->getProt() & PROT_WRITE ) {
        perms |= 0x2;
    }

    // ***************
    // Note that we are not flushing the TLBs
    // to this point in development calls to mprotect have been limited and we can get away with not flushing TLBs
    // ***************
    process->mprotect( event->getAddr(), event->getLen(), perms );

    setReturnSuccess(0);
}
