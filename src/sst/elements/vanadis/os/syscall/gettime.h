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

#ifndef _H_VANADIS_OS_SYSCALL_GETTIME
#define _H_VANADIS_OS_SYSCALL_GETTIME

#include "os/syscall/syscall.h"
#include "os/callev/voscallgettime64.h"

namespace SST {
namespace Vanadis {

class VanadisGettime64Syscall : public VanadisSyscall {
public:
    VanadisGettime64Syscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, uint64_t sim_time_ns, VanadisSyscallGetTime64Event* event )
        : VanadisSyscall( os, coreLink, process, event, "gettime64" )
    {
        uint64_t sim_seconds = (uint64_t)(sim_time_ns / 1000000000ULL);
        uint32_t sim_ns = (uint32_t)(sim_time_ns % 1000000000ULL);

        m_output->verbose(CALL_INFO, 16, 0,
                            "[syscall-gettime64] --> sim-time: %" PRIu64 " ns -> %" PRIu64 " secs + %" PRIu32 " us\n",
                            sim_time_ns, sim_seconds, sim_ns);


        if ( VanadisOSBitType::VANADIS_OS_64B == event->getOSBitType() ) {
            payload.resize( 16 );
            uint64_t* tv_sec  = (uint64_t*) payload.data();
            uint64_t* tv_nsec = tv_sec + 1;
            // does this work for big and little endian?
            *tv_sec  = sim_seconds;
            *tv_nsec = sim_ns;
        } else {
            payload.resize( 12 );
            uint64_t* tv_sec  = (uint64_t*) payload.data();
            uint32_t* tv_nsec = (uint32_t*) (tv_sec + 1);
            // does this work for big and little endian?
            *tv_sec  = sim_seconds;
            *tv_nsec = sim_ns;
        }
        writeMemory( event->getTimeStructAddress(), payload );
    }

    void memReqIsDone(bool) {
        setReturnSuccess(0);
    }

private:
    std::vector<uint8_t> payload;
};

} // namespace Vanadis
} // namespace SST

#endif
