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

#ifndef _H_VANADIS_OS_SYSCALL_FORK
#define _H_VANADIS_OS_SYSCALL_FORK

#include "os/syscall/syscall.h"
#include "os/callev/voscallfork.h"
#include "os/include/hwThreadID.h"

namespace SST {
namespace Vanadis {

class VanadisNodeOSComponent;
class VanadisGetThreadStateResp;
class VanadisForkSyscall : public VanadisSyscall {
public:
    VanadisForkSyscall( Output* output, Link* link, OS::ProcessInfo* process, SendMemReqFunc* func, VanadisSyscallForkEvent* event, VanadisNodeOSComponent* );
    ~VanadisForkSyscall() { 
        delete m_threadID;
    }
    void complete( VanadisGetThreadStateResp* ); 

 private:  
    OS::ProcessInfo* m_child;  
    VanadisNodeOSComponent* m_os;
    OS::HwThreadID* m_threadID;
};

} // namespace Vanadis
} // namespace SST

#endif