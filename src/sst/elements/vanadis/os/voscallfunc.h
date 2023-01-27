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

#ifndef _H_VANADIS_SYSTEM_CALL_FUNC_TYPE
#define _H_VANADIS_SYSTEM_CALL_FUNC_TYPE

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

#define FOREACH_FUNCTION(NAME) \
    NAME(SYSCALL_OP_UNKNOWN)\
    NAME(SYSCALL_OP_ACCESS)\
    NAME(SYSCALL_OP_BRK)\
    NAME(SYSCALL_OP_SET_THREAD_AREA)\
    NAME(SYSCALL_OP_FORK)\
    NAME(SYSCALL_OP_CLONE)\
    NAME(SYSCALL_OP_FUTEX)\
    NAME(SYSCALL_OP_GETPID)\
    NAME(SYSCALL_OP_GETTID)\
    NAME(SYSCALL_OP_GETPPID)\
    NAME(SYSCALL_OP_GETPGID)\
    NAME(SYSCALL_OP_UNAME)\
    NAME(SYSCALL_OP_OPENAT)\
    NAME(SYSCALL_OP_OPEN)\
    NAME(SYSCALL_OP_CLOSE)\
    NAME(SYSCALL_OP_READV)\
    NAME(SYSCALL_OP_READ)\
    NAME(SYSCALL_OP_READLINK)\
    NAME(SYSCALL_OP_WRITEV)\
    NAME(SYSCALL_OP_WRITE)\
    NAME(SYSCALL_OP_IOCTL)\
    NAME(SYSCALL_OP_FSTAT)\
    NAME(SYSCALL_OP_STATX)\
    NAME(SYSCALL_OP_MMAP)\
    NAME(SYSCALL_OP_MMAPX)\
    NAME(SYSCALL_OP_UNMAP)\
    NAME(SYSCALL_OP_MPROTECT)\
    NAME(SYSCALL_OP_EXIT)\
    NAME(SYSCALL_OP_EXIT_GROUP)\
    NAME(SYSCALL_OP_GETTIME64)\
    NAME(SYSCALL_OP_UNLINKAT)\
    NAME(SYSCALL_OP_UNLINK)\
    NAME(SYSCALL_OP_SET_TID_ADDRESS)\
    NAME(NUM_SYSCALLS)\

namespace SST {
namespace Vanadis {

enum VanadisSyscallOp {
    FOREACH_FUNCTION(GENERATE_ENUM)
};

}
} // namespace SST

#endif
