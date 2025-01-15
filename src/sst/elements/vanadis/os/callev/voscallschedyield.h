// os/callev/voscallschedyield.h
#ifndef _H_VANADIS_SYSCALL_SCHED_YIELD
#define _H_VANADIS_SYSCALL_SCHED_YIELD

#include "os/voscallev.h"
#include "os/vosbittype.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallSchedYieldEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallSchedYieldEvent() : VanadisSyscallEvent() {}

    VanadisSyscallSchedYieldEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype)
        : VanadisSyscallEvent(core, thr, bittype) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_SCHED_YIELD; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallSchedYieldEvent);
};

} // namespace Vanadis
} // namespace SST

#endif
