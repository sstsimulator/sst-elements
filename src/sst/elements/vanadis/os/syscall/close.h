// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OS_SYSCALL_CLOSE
#define _H_VANADIS_OS_SYSCALL_CLOSE

#include "os/syscall/syscall.h"
#include "os/callev/voscallclose.h"

namespace SST {
namespace Vanadis {

class VanadisCloseSyscall : public VanadisSyscall {
public:
    VanadisCloseSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallCloseEvent* event ) 
        : VanadisSyscall( os, coreLink, process, event, "close" ) 
    {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-close] -> call is close( %" PRIu32 " )\n", event->getFileDescriptor());

        auto retval = process->closeFile( event->getFileDescriptor() );
        if ( retval < 0 ) {
            setReturnFail(retval);
        } else {
            setReturnSuccess(0);
        }
    }
};

} // namespace Vanadis
} // namespace SST

#endif
