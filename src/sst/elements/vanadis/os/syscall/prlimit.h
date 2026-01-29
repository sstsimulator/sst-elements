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

#ifndef _H_VANADIS_OS_SYSCALL_PRLIMIT
#define _H_VANADIS_OS_SYSCALL_PRLIMIT

#include "os/syscall/syscall.h"
#include "os/callev/voscallprlimit.h"

namespace SST {
namespace Vanadis {

#define VANADIS_RLIMIT_STACK        3   /* max stack size */

typedef uint64_t vanadis_rlim_t;
struct vanadis_rlimit {
    vanadis_rlim_t rlim_cur;  /* Soft limit */
    vanadis_rlim_t rlim_max;  /* Hard limit (ceiling for rlim_cur) */
};

class VanadisPrlimitSyscall : public VanadisSyscall {
public:
    VanadisPrlimitSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallPrlimitEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "prlimit" ), m_state( READ )
    {
        #ifdef VANADIS_BUILD_DEBUG
        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-prlimit] pid=%" PRIu64 " resource=%" PRIu64 " new_limit=%#" PRIx64 " old_limit=%#" PRIx64 "\n",
            event->getPid(),event->getResource(),event->getNewLimit(),event->getOldLimit());
        #endif
        assert( event->getResource() == VANADIS_RLIMIT_STACK );

        payload.resize( sizeof( struct vanadis_rlimit ) );

        if ( event->getNewLimit() ) {
            readMemory( event->getNewLimit(), payload );
        } else {
            writeOld();
        }
    }

    void memReqIsDone(bool) {
        if ( READ == m_state ) {
            m_state = WRITE;
            auto buf = ( struct vanadis_rlimit * ) payload.data();
            printf("[syscall-prlimit] newlimit %" PRIu64 " %" PRIu64 "\n",buf->rlim_cur,buf->rlim_max);

            writeOld();
        } else {
            setReturnSuccess(0);
        }
    }

    void writeOld() {
        auto event = getEvent<VanadisSyscallPrlimitEvent*>();

        if ( event->getOldLimit() ) {
            auto buf = ( struct vanadis_rlimit * ) payload.data();
            buf->rlim_cur = 1024*1014*10;
            buf->rlim_max = -1;

            m_state = WRITE;
            writeMemory( event->getOldLimit(), payload );
        } else {
            setReturnSuccess(0);
        }
    }

 private:
    enum { READ, WRITE } m_state;
    std::vector<uint8_t> payload;
};

} // namespace Vanadis
} // namespace SST

#endif
