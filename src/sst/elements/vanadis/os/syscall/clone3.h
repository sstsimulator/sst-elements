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

#ifndef _H_VANADIS_OS_SYSCALL_CLONE3
#define _H_VANADIS_OS_SYSCALL_CLONE3

#include "os/syscall/syscall.h"
#include "os/callev/voscallclone3.h"

#include <cstdint>
#include <vector>

namespace SST {
namespace Vanadis {

class VanadisClone3Syscall : public VanadisSyscall {
public:
    VanadisClone3Syscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallClone3Event* event );
    ~VanadisClone3Syscall() {
        delete hw_thread_id_;
    }
    void handleEvent( VanadisCoreEvent* ev );

 private:

    enum class State { ReadCloneArgs, ChildSetTid, ThreadArgs } state_;

    void memReqIsDone(bool);
    void parseCloneArgs( VanadisSyscallClone3Event* );
    void setTidAtParent( VanadisSyscallClone3Event* );
    void finish( VanadisSyscallClone3Event* );

    OS::HwThreadID* hw_thread_id_ = nullptr;
    OS::ProcessInfo* new_thread_ = nullptr;

    // Buffer for read/write operations
    std::vector<uint8_t> buffer_;

    // Arg struct members
    uint64_t flags_ = 0;        /* Flags bit mask */
    uint64_t pidfd_ = 0;        /* Where to store PID file descriptor (int*) */
    uint64_t child_tid_ = 0;    /* Where to store child TID in child's memory (pid_t*) */
    uint64_t parent_tid_ = 0;   /* Where to store child TID, in parent's memory (pid_t*) */
    uint64_t exit_signal_ = 0;  /* Signal to deliver to parent on child termination */
    uint64_t stack_ = 0;        /* Pointer to lowest byte of stack */
    uint64_t stack_size_ = 0;   /* Size of stack */
    uint64_t tls_ = 0;          /* Location of new TLS */
    uint64_t set_tid_ = 0;      /* Pointer to a pid_t array */
    uint64_t set_tid_size_ = 0; /* Number of elements in set_tid */
    uint64_t cgroup_ = 0;       /* File descriptor for target cgroup of child */
};

} // namespace Vanadis
} // namespace SST

#endif
