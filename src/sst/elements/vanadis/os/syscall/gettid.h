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

#ifndef _H_VANADIS_OS_SYSCALL_GETTID
#define _H_VANADIS_OS_SYSCALL_GETTID

#include "os/syscall/syscall.h"
#include "os/callev/voscallgetx.h"

namespace SST {
namespace Vanadis {

class VanadisGettidSyscall : public VanadisSyscall {
public:
    VanadisGettidSyscall( Output* output, Link* link, OS::ProcessInfo* process, SendMemReqFunc* func, VanadisSyscallGetxEvent* event )
        : VanadisSyscall( output, link, process, func, event, "gettid" ) 
    {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-gettid]\n");
        setReturnSuccess(process->gettid());
    }
};

} // namespace Vanadis
} // namespace SST

#endif
