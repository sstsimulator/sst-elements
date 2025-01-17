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

#ifndef _H_VANADIS_OS_SYSCALL_FORK
#define _H_VANADIS_OS_SYSCALL_FORK

#include "os/syscall/syscall.h"
#include "os/callev/voscallfork.h"
#include "os/include/hwThreadID.h"

namespace SST {
namespace Vanadis {

class VanadisForkSyscall : public VanadisSyscall {
public:
    VanadisForkSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallForkEvent* event );
    ~VanadisForkSyscall() { 
        delete m_threadID;
    }
    void handleEvent( VanadisCoreEvent* ev );

 private:  
    OS::ProcessInfo* m_child;  
    OS::HwThreadID* m_threadID;
};

} // namespace Vanadis
} // namespace SST

#endif
