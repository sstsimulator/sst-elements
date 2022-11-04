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

#ifndef _H_VANADIS_NODE_OS_STAT_STRUCT
#define _H_VANADIS_NODE_OS_STAT_STRUCT

#include <cinttypes>
#include <cstdint>

#include <unistd.h>

namespace SST {
namespace Vanadis {

struct vanadis_timespec32 {
    int32_t tv_sec;
    int32_t tv_nsec;
};
struct vanadis_timespec64 {
    int64_t tv_sec;
    int64_t tv_nsec;
};

/*
struct stat {
        dev_t st_dev;                   int64_t
        long __st_padding1[2];	        long[2]
        ino_t st_ino;	                uint64_t
        mode_t st_mode;	                unsigned
        nlink_t st_nlink;	        unsigned
        uid_t st_uid;	                unsigned
        gid_t st_gid;	                unsigned
        dev_t st_rdev;	                uint64_t
        long __st_padding2[2];	        long[2]
        off_t st_size;	                int64_t
        struct {
                long tv_sec;	        long
                long tv_nsec;	        long
        } __st_atim32, __st_mtim32, __st_ctim32;
        blksize_t st_blksize;	        int
        long __st_padding3;	        long
        blkcnt_t st_blocks;	        int64_t
        struct timespec st_atim;        { long,	long }
        struct timespec st_mtim;
        struct timespec st_ctim;
        long __st_padding4[2];	        long[2]
};
*/

struct vanadis_stat32 {
    int64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    int64_t st_size;
    int32_t st_blksize;
    int64_t st_blocks;
    vanadis_timespec32 st_atim;
    vanadis_timespec32 st_mtim;
    vanadis_timespec32 st_ctim;
};



/* riscv64
struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    unsigned long __pad;
    off_t st_size;
    blksize_t st_blksize;
    int __pad2;
    blkcnt_t st_blocks;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    unsigned __unused[2];
};
*/

struct vanadis_stat64 {
    uint64_t st_dev;
    int64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t  st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    unsigned long __pad;
    uint64_t st_size;
    uint32_t st_blksize;
    int __pad2;
    uint64_t st_blocks;
    vanadis_timespec64 st_atim;
    vanadis_timespec64 st_mtim;
    vanadis_timespec64 st_ctim;
    unsigned __unused[2];
};

template <typename T>
void
vanadis_copy_native_stat(const struct stat* native_stat, T* v_stat) {
    bzero( v_stat, sizeof( T ) );
    v_stat->st_dev = native_stat->st_dev;
    v_stat->st_ino = native_stat->st_ino;
    v_stat->st_mode = (uint32_t)native_stat->st_mode;
    v_stat->st_nlink = (uint32_t)native_stat->st_nlink;
    v_stat->st_uid = (uint32_t)native_stat->st_uid;
    v_stat->st_gid = (uint32_t)native_stat->st_gid;
    v_stat->st_rdev = native_stat->st_rdev;
    v_stat->st_size = native_stat->st_size;
    v_stat->st_blksize = (int32_t)native_stat->st_blksize;
    v_stat->st_blocks = native_stat->st_blocks;
#ifdef SST_COMPILE_MACOSX
    v_stat->st_atim.tv_sec = (int32_t)native_stat->st_atimespec.tv_sec;
    v_stat->st_atim.tv_nsec = (int32_t)native_stat->st_atimespec.tv_nsec;
    v_stat->st_mtim.tv_sec = (int32_t)native_stat->st_mtimespec.tv_sec;
    v_stat->st_mtim.tv_nsec = (int32_t)native_stat->st_mtimespec.tv_nsec;
    v_stat->st_ctim.tv_sec = (int32_t)native_stat->st_ctimespec.tv_sec;
    v_stat->st_ctim.tv_nsec = (int32_t)native_stat->st_ctimespec.tv_nsec;
#else
    v_stat->st_atim.tv_sec = (int32_t)native_stat->st_atim.tv_sec;
    v_stat->st_atim.tv_nsec = (int32_t)native_stat->st_atim.tv_nsec;
    v_stat->st_mtim.tv_sec = (int32_t)native_stat->st_mtim.tv_sec;
    v_stat->st_mtim.tv_nsec = (int32_t)native_stat->st_mtim.tv_nsec;
    v_stat->st_ctim.tv_sec = (int32_t)native_stat->st_ctim.tv_sec;
    v_stat->st_ctim.tv_nsec = (int32_t)native_stat->st_ctim.tv_nsec;
#endif
};

} // namespace Vanadis
} // namespace SST

#endif
