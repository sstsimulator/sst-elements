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

namespace SST {
namespace Vanadis {

enum VanadisSyscallOp {
    SYSCALL_OP_UNKNOWN,
    SYSCALL_OP_ACCESS,
    SYSCALL_OP_BRK,
    SYSCALL_OP_SET_THREAD_AREA,
    SYSCALL_OP_UNAME,
    SYSCALL_OP_OPENAT,
    SYSCALL_OP_OPEN,
    SYSCALL_OP_CLOSE,
    SYSCALL_OP_READ,
    SYSCALL_OP_READLINK,
    SYSCALL_OP_WRITEV,
    SYSCALL_OP_WRITE,
    SYSCALL_OP_IOCTL,
    SYSCALL_OP_FSTAT,
    SYSCALL_OP_MMAP,
    SYSCALL_OP_MMAPX,
    SYSCALL_OP_UNMAP,
    SYSCALL_OP_EXIT,
    SYSCALL_OP_EXIT_GROUP,
    SYSCALL_OP_GETTIME64
};

}
} // namespace SST

#endif
