
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
	int64_t  st_dev;
	uint64_t st_ino;
	uint32_t st_mode;
	uint32_t st_nlink;
	uint32_t st_uid;
	uint32_t st_gid;
	uint64_t st_rdev;
	int64_t	 st_size;
	int32_t  st_blksize;
	int64_t st_blocks;
	vanadis_timespec32 st_atim;
	vanadis_timespec32 st_mtim;
	vanadis_timespec32 st_ctim;
};

struct vanadis_stat64 {

};

void vanadis_copy_native_stat( const struct stat* native_stat, struct vanadis_stat32* v_stat ) {
	v_stat->st_dev = native_stat->st_dev;
	v_stat->st_ino = native_stat->st_ino;
	v_stat->st_mode = (uint32_t) native_stat->st_mode;
	v_stat->st_nlink = (uint32_t) native_stat->st_nlink;
	v_stat->st_uid = (uint32_t) native_stat->st_uid;
	v_stat->st_gid = (uint32_t) native_stat->st_gid;
	v_stat->st_rdev = native_stat->st_rdev;
	v_stat->st_size = native_stat->st_size;
	v_stat->st_blksize = (int32_t) native_stat->st_blksize;
	v_stat->st_blocks = native_stat->st_blocks;
	v_stat->st_atim.tv_sec = (int32_t) native_stat->st_atim.tv_sec;
	v_stat->st_atim.tv_nsec = (int32_t) native_stat->st_atim.tv_nsec;
	v_stat->st_mtim.tv_sec = (int32_t) native_stat->st_mtim.tv_sec;
	v_stat->st_mtim.tv_nsec = (int32_t) native_stat->st_mtim.tv_nsec;
	v_stat->st_ctim.tv_sec = (int32_t) native_stat->st_ctim.tv_sec;
	v_stat->st_ctim.tv_nsec = (int32_t) native_stat->st_ctim.tv_nsec;
};

}
}

#endif
