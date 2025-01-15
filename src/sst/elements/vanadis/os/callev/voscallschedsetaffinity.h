#ifndef _H_VANADIS_SYSCALL_SCHED_SET_AFFINITY
#define _H_VANADIS_SYSCALL_SCHED_SET_AFFINITY

#include "os/voscallev.h"
#include "os/vosbittype.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallSchedSetAffinityEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallSchedSetAffinityEvent() : VanadisSyscallEvent() {}
    VanadisSyscallSchedSetAffinityEvent(
        uint32_t core, uint32_t thr, VanadisOSBitType bitType,
        uint64_t pid, uint64_t cpusetsize, uint64_t maskAddr)
      : VanadisSyscallEvent(core, thr, bitType),
        m_pid(pid), m_cpusetsize(cpusetsize), m_maskAddr(maskAddr)
    {}

    VanadisSyscallOp getOperation() override {
        return SYSCALL_OP_SCHED_SETAFFINITY;
    }

    uint64_t getPid()         const { return m_pid; }
    uint64_t getCpuSetSize()  const { return m_cpusetsize; }
    uint64_t getMaskAddr()    const { return m_maskAddr; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        ser & m_pid;
        ser & m_cpusetsize;
        ser & m_maskAddr;
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallSchedSetAffinityEvent);

    uint64_t m_pid;
    uint64_t m_cpusetsize;
    uint64_t m_maskAddr;
};

} // namespace Vanadis
} // namespace SST

#endif
