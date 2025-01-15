#ifndef _H_VANADIS_OS_SYSCALL_SCHED_SET_AFFINITY
#define _H_VANADIS_OS_SYSCALL_SCHED_SET_AFFINITY

#include "os/syscall/syscall.h"
#include "os/callev/voscallschedsetaffinity.h"

namespace SST {
namespace Vanadis {

class VanadisSchedSetAffinitySyscall : public VanadisSyscall {
public:
    VanadisSchedSetAffinitySyscall(
        VanadisNodeOSComponent* os,
        SST::Link* coreLink,
        OS::ProcessInfo* process,
        VanadisSyscallSchedSetAffinityEvent* event
    ) : VanadisSyscall(os, coreLink, process, event, "sched_setaffinity"),
        m_pid(event->getPid()),
        m_cpusetsize(event->getCpuSetSize()),
        m_maskAddr(event->getMaskAddr())
    {
        m_output->verbose(CALL_INFO, 16, 0,
            "[syscall-sched_setaffinity] -> read mask from 0x%" PRIx64
            " (size=%" PRIu64 "), pid=%" PRId64 "\n",
            m_maskAddr, m_cpusetsize, m_pid);

        // If we need to read the CPU mask from user memory:
        if (m_cpusetsize > 0 && m_maskAddr != 0) {
            m_cpumask.resize(m_cpusetsize);
            // readMemory() is inherited from VanadisSyscall
            // to request the data from the CPU’s memory
            readMemory(m_maskAddr, m_cpumask, m_cpusetsize);
        } else {
            // If cpusetsize=0 or maskAddr=0 => immediate error or success.
            finalizeSyscall();
        }
    }

    // If we do read memory, the OS will eventually call this once data arrives.
    void memReqIsDone(bool success) override {
        if (!success) {
            // Couldn't read memory?
            setReturnFail(-EFAULT);
            return;
        }
        // Here we have m_cpumask fully read from user memory
        m_output->verbose(CALL_INFO, 16, 0,
            "[syscall-sched_setaffinity] -> got mask data (size=%zu)\n",
            m_cpumask.size());
        // For demonstration, let's do a minimal approach:
        finalizeSyscall();
    }

private:
    void finalizeSyscall() {
        // Real code might parse m_cpumask bits, check if they're valid,
        // possibly set CPU affinity for the process with pid=m_pid.
        // For now, just setReturnSuccess(0).
        // If you want to emulate partial success or do real scheduling logic,
        // you'd do it here.

        setReturnSuccess(0);
    }

    uint64_t m_pid;
    uint64_t m_cpusetsize;
    uint64_t m_maskAddr;
    std::vector<uint8_t> m_cpumask;
};

} // namespace Vanadis
} // namespace SST

#endif
