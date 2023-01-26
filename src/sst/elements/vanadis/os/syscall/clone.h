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

#ifndef _H_VANADIS_OS_SYSCALL_CLONE
#define _H_VANADIS_OS_SYSCALL_CLONE

#include "os/syscall/syscall.h"
#include "os/callev/voscallclone.h"

namespace SST {
namespace Vanadis {

class VanadisCloneSyscall : public VanadisSyscall {
public:
    VanadisCloneSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallCloneEvent* event );
    ~VanadisCloneSyscall() {
        delete m_threadID;
    }
    void handleEvent( VanadisCoreEvent* ev );

 private:  

    enum State { ReadChildTidAddr, ReadThreadStack, WriteChildTid } m_state;

    void memReqIsDone();

    uint64_t m_threadStartAddr;
    uint64_t m_threadArgAddr;

    OS::HwThreadID* m_threadID;
    OS::ProcessInfo* m_newThread;

    std::vector<uint8_t> m_buffer;
};

} // namespace Vanadis
} // namespace SST

#endif
