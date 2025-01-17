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

#ifndef _H_VANADIS_OS_SYSCALL_FSTATAT
#define _H_VANADIS_OS_SYSCALL_FSTATAT

#include "os/syscall/syscall.h"
#include "os/callev/voscallfstatat.h"

#if 0
struct timespec_64 {
    uint64_t tv_sec;
    uint64_t tv_nsec;
};
struct stat_64 {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t st_size;
    uint32_t st_blksize;
    uint64_t st_blocks;
    struct timespec_64 st_atim;
    struct timespec_64 st_mtim;
    struct timespec_64 st_ctim;
};

struct timespec_32 {
    uint64_t tv_sec;
    uint32_t tv_nsec;
};
struct stat_32 {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t st_size;
    uint32_t st_blksize;
    uint64_t st_blocks;
    struct timespec_32 st_atim;
    struct timespec_32 st_mtim;
    struct timespec_32 st_ctim;
};
#endif


#define VANDAID_AT_EMPTY_PATH 0x1000

namespace SST {
namespace Vanadis {

class VanadisFstatatSyscall : public VanadisSyscall {
public:
    VanadisFstatatSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallFstatAtEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "fstatat" ), m_state( READ ) 
    {
	    m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-fstatat] dirfd=%" PRIu64 " pathaname=%#" PRIx64 " statbuf=%#" PRIx64 " flags=%#" PRIx64 "\n",
                event->getDirfd(), event->getPathname(), event->getStatbuf(), event->getFlags() );

        setReturnFail(-LINUX_EINVAL);
#if 0
        assert( event->getFlags() == VANDAID_AT_EMPTY_PATH );
        m_dirFd  = event->getDirfd();
        // if the directory fd passed by the syscall is positive it should point to a entry in the file_descriptor table
        // if the directory fd is negative pass that to to the openat handler ( AT_FDCWD is negative )
        if ( m_dirFd > -1 ) {
            m_dirFd = process->getFileDescriptor( m_dirFd );

            if ( -1 == m_dirFd) {
                m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "can't find m_dirFd=%d in open file descriptor table\n", m_dirFd);
                setReturnFail(-LINUX_EBADF);
                return;
            }
            // get the FD that SST will use
            m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "sst fd=%d pathname=%s\n", m_dirFd, process->getFilePath(m_dirFd).c_str());
        }

        readString(event->getPathname(),m_filename);
#endif
    }

#if 0
    void memReqIsDone(bool) {
        auto event = getEvent<VanadisSyscallFstatAtEvent*>();
        if ( READ == m_state) {
            m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-fstatat] path: \"%s\"\n", m_filename.c_str());

            if ( 0 == m_filename[0] && event->getFlags() == VANDAID_AT_EMPTY_PATH ) {
                m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-fstatat] dirfd %d as fd\n",event->getDirfd());
            }
        }
    }
#endif

 #if 0       
	    auto fd = process->getFileDescriptor( event->getFileHandle());
        if (-1 == fd ) {
            m_output->verbose(CALL_INFO, 16, 0,
                                "[syscall-fstatat] -> file handle %" PRId64
                                " is not currently open, return an error code.\n",
                                event->getFileHandle());

        } else {

            if ( event->getFileHandle() > 2 ) {
               m_output->fatal(CALL_INFO, -1, "Error: fstatat currently only supports fd 1 and 2, can't fstatat fd=%d\n",event->getFileHandle());
            }
            if ( VanadisOSBitType::VANADIS_OS_32B == event->getOSBitType() ) {
                statBuf.resize(sizeof(struct stat_32));
            } else {
                statBuf.resize(sizeof(struct stat_64));
            }
            setReturnSuccess(0);
#endif            
#if 0
            if (  <= 2) {
                if (0 == fstatat(fstatat_ev->getFileHandle(), &stat_output)) {
                    vanadis_copy_native_stat(&stat_output, &v32_stat_output);
                    success = true;
                } else {
                    fstatat_return_code = -13;
                }
            } else {
                auto fd = process->getFileDescriptor( fstatat_ev->getFileHandle() );

                if ( -1 == fd ) {
                    // Fail - cannot find the descriptor, so its not open
                    fstatat_return_code = -9;
                } else {

                    if (0 == fstatat(fd, &stat_output)) {
                        vanadis_copy_native_stat(&stat_output, &v32_stat_output);
                        success = true;
                    } else {
                        fstatat_return_code = -13;
                    }
                }
            }
        }
    }
#endif

#if 0 // not implemented

            bool success = false;
            struct vanadis_stat32 v32_stat_output;
            struct stat stat_output;

                stat_write_payload.reserve(sizeof(v32_stat_output));

                uint8_t* stat_output_ptr = (uint8_t*)(&v32_stat_output);
                for (int i = 0; i < sizeof(stat_output); ++i) {
                    stat_write_payload.push_back(stat_output_ptr[i]);
                }
#endif

private:
    enum { READ, WRITE }            m_state;

#if 0
    std::vector<uint8_t> statBuf;
    int                             m_dirFd;
    std::string                     m_filename;
    std::vector<uint8_t>            m_data;
#endif
};

} // namespace Vanadis
} // namespace SST

#endif
