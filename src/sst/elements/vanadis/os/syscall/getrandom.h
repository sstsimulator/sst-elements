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

#ifndef _H_VANADIS_OS_SYSCALL_GETRANDOM
#define _H_VANADIS_OS_SYSCALL_GETRANDOM

#include "os/syscall/syscall.h"
#include "os/callev/voscallgetrandom.h"
#include <vector>
#include <sst/core/rng/xorshift.h>


namespace SST {
namespace Vanadis {

class VanadisGetrandomSyscall : public VanadisSyscall {
public:
    VanadisGetrandomSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallGetrandomEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "getrandom" )
    {
        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-getrandom] buf=%#" PRIx64 " buflen=%" PRIu64 " flags=%#" PRIx64 "\n",
            event->getBuf(),event->getBuflen(),event->getFlags());

        payload.resize( event->getBuflen() );

        RNG::XORShiftRNG rng(272727);

        for ( int i = 0; i < payload.size(); ++i ) {
            auto val = rng.generateNextUInt32() % 255;
            payload[i] = val;
        }

        writeMemory( event->getBuf(), payload );
    }

    void memReqIsDone(bool) {
        setReturnSuccess( payload.size() );
    }

 private:
    std::vector<uint8_t> payload;
};

} // namespace Vanadis
} // namespace SST

#endif
