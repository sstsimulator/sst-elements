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

#ifndef _H_VANADIS_OS_SYSCALL_SET_TID_ADDRESS
#define _H_VANADIS_OS_SYSCALL_SET_TID_ADDRESS

#include "os/syscall/syscall.h"
#include "os/callev/voscallsettidaddr.h"

namespace SST {
namespace Vanadis {

class VanadisSetTidAddressSyscall : public VanadisSyscall {
public:
    VanadisSetTidAddressSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallSetTidAddressEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "set_tid_address" ) 
    {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-set_tid_address] address %#" PRIx64 "\n", event->getTidAddress());

        process->setTidAddress( event->getTidAddress() );

        setReturnSuccess(0);
    }
};

} // namespace Vanadis
} // namespace SST

#endif
