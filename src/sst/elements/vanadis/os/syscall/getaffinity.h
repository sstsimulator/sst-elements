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

#ifndef _H_VANADIS_OS_SYSCALL_GETAFFINITY
#define _H_VANADIS_OS_SYSCALL_GETAFFINITY

#include "os/syscall/syscall.h"
#include "os/callev/voscallgetaffinity.h"

namespace SST {
namespace Vanadis {

#define __CPU_op_S(i, size, set, op) ( (i)/8U >= (size) ? 0 : \
    (((unsigned long *)(set))[(i)/8/sizeof(long)] op (1UL<<((i)%(8*sizeof(long))))) )

class VanadisGetaffinitySyscall : public VanadisSyscall {
public:
    VanadisGetaffinitySyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallGetaffinityEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "getaffinity" ) 
    {
        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-getaffinity] uname pid=%" PRIu64 " cpusetsize=%" PRIu64 " maskAddr=%#" PRIx64 "\n",
                                            event->getPid(), event->getCpusetsize(), event->getMaskAddr());

#if 0 
typedef struct cpu_set_t { unsigned long __bits[128/sizeof(long)]; } cpu_set_t;
#endif
        assert( 0 == event->getPid() );
        // for now set every CPU available for scheduling
        int num_cores = os->getCoreCount();

        // Resize payload to represent 128 bits (16 bytes)
        payload.resize(sizeof(uint64_t) * (128 / sizeof(uint64_t)), 0x00);

        // Set the first 'num_cores' bits to 1
        uint64_t* cpu_set = reinterpret_cast<uint64_t*>(payload.data());
        for (int i = 0; i < num_cores; i++) {
            int element_index = i / 64;    // Determine which uint64_t element
            int bit_position = i % 64;    // Determine the bit position within the element
            cpu_set[element_index] |= (1ULL << bit_position); // Set the bit to 1
        }

        // Write the updated payload to memory
        writeMemory(event->getMaskAddr(), payload);

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
