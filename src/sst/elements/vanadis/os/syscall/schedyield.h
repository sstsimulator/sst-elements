#ifndef _H_VANADIS_OS_SYSCALL_SCHED_YIELD
#define _H_VANADIS_OS_SYSCALL_SCHED_YIELD

#include "os/syscall/syscall.h"
#include "os/callev/voscallschedyield.h"
#include <iostream>

namespace SST {
namespace Vanadis {

class VanadisSchedYieldSyscall : public VanadisSyscall {
public:
    VanadisSchedYieldSyscall(
        VanadisNodeOSComponent* os, SST::Link* coreLink, 
        OS::ProcessInfo* process,
        VanadisSyscallSchedYieldEvent* event)
    : VanadisSyscall(os, coreLink, process, event, "sched_yield") 
    {
        // No arguments to decode
        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL,
            "[syscall-sched_yield] -> sched_yield()\n");
        setReturnSuccess(0);
    }
};

} // namespace Vanadis
} // namespace SST

#endif
