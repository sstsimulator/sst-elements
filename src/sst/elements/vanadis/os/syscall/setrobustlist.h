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

#ifndef _H_VANADIS_OS_SYSCALL_SET_ROBUST_LIST
#define _H_VANADIS_OS_SYSCALL_SET_ROBUST_LIST

#include "os/syscall/syscall.h"
#include "os/callev/voscallsetrobustlist.h"

namespace SST {
namespace Vanadis {

class VanadisSetRobustListSyscall : public VanadisSyscall {
public:
    VanadisSetRobustListSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallSetRobustListEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "set_robust_list" )
    {
        #ifdef VANADIS_BUILD_DEBUG
        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-set_robust_list] head %#" PRIx64 ", len=%" PRIu64 "\n", event->getHead(),event->getLen());
        #endif
//        process->setRobustList( event->getRobustList() );

        setReturnSuccess(0);
    }
};

} // namespace Vanadis
} // namespace SST

#endif
