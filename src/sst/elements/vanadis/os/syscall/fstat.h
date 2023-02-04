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

#ifndef _H_VANADIS_OS_SYSCALL_FSTAT
#define _H_VANADIS_OS_SYSCALL_FSTAT

#include "os/syscall/syscall.h"
#include "os/callev/voscallfstat.h"

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

namespace SST {
namespace Vanadis {

class VanadisFstatSyscall : public VanadisSyscall {
public:
    VanadisFstatSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallFstatEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "fstat" ) 
    {
	    m_output->verbose(CALL_INFO, 16, 0, "-> call is fstat( %d, 0x%0llx )\n", event->getFileHandle(), event->getStructAddress());

        setReturnFail(-EINVAL);

 #if 0       
	    auto fd = process->getFileDescriptor( event->getFileHandle());
        if (-1 == fd ) {
            m_output->verbose(CALL_INFO, 16, 0,
                                "[syscall-fstat] -> file handle %" PRId64
                                " is not currently open, return an error code.\n",
                                event->getFileHandle());

        } else {

            if ( event->getFileHandle() > 2 ) {
               m_output->fatal(CALL_INFO, -1, "Error: fstat currently only supports fd 1 and 2, can't fstat fd=%d\n",event->getFileHandle());
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
                if (0 == fstat(fstat_ev->getFileHandle(), &stat_output)) {
                    vanadis_copy_native_stat(&stat_output, &v32_stat_output);
                    success = true;
                } else {
                    fstat_return_code = -13;
                }
            } else {
                auto fd = process->getFileDescriptor( fstat_ev->getFileHandle() );

                if ( -1 == fd ) {
                    // Fail - cannot find the descriptor, so its not open
                    fstat_return_code = -9;
                } else {

                    if (0 == fstat(fd, &stat_output)) {
                        vanadis_copy_native_stat(&stat_output, &v32_stat_output);
                        success = true;
                    } else {
                        fstat_return_code = -13;
                    }
                }
            }
        }
#endif
    }

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
    std::vector<uint8_t> statBuf;
};

} // namespace Vanadis
} // namespace SST

#endif
