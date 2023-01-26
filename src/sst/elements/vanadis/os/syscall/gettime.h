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

#ifndef _H_VANADIS_OS_SYSCALL_GETTIME
#define _H_VANADIS_OS_SYSCALL_GETTIME

#include "os/syscall/syscall.h"
#include "os/callev/voscallgettime64.h"

namespace SST {
namespace Vanadis {

class VanadisGettime64Syscall : public VanadisSyscall {
public:
    VanadisGettime64Syscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, uint64_t sim_time_ns, VanadisSyscallGetTime64Event* event )
        : VanadisSyscall( os, coreLink, process, event, "gettime64" ), m_state(0)
    {
        uint64_t sim_seconds = (uint64_t)(sim_time_ns / 1000000000ULL);
        uint32_t sim_ns = (uint32_t)(sim_time_ns % 1000000000ULL);

        m_output->verbose(CALL_INFO, 16, 0,
                            "[syscall-gettime64] --> sim-time: %" PRIu64 " ns -> %" PRIu64 " secs + %" PRIu32 " us\n",
                            sim_time_ns, sim_seconds, sim_ns);

        seconds_payload.resize(sizeof(sim_seconds),0);
        ns_payload.resize(sizeof(sim_ns),0);

        uint8_t* sec_ptr = (uint8_t*)&sim_seconds;
        for (size_t i = 0; i < sizeof(sim_seconds); ++i) {
            seconds_payload[i] = sec_ptr[i];
        }

        uint8_t* ns_ptr = (uint8_t*)&sim_ns;
        for (size_t i = 0; i < sizeof(sim_ns); ++i) {
            ns_payload[i] = ns_ptr[i];
        }

        writeMemory( event->getTimeStructAddress(), seconds_payload );
    }

    void memReqIsDone() { 
        if ( 0 == m_state ) {
            m_state = 1;
            writeMemory( getEvent<VanadisSyscallGetTime64Event*>()->getTimeStructAddress() + seconds_payload.size(), ns_payload );
        } else if ( 1 == m_state ) {
            setReturnSuccess(0);
        }
    }

private:
    int m_state;
    std::vector<uint8_t> seconds_payload;
    std::vector<uint8_t> ns_payload;
};

} // namespace Vanadis
} // namespace SST

#endif
