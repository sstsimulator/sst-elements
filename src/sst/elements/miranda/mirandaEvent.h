#ifndef _H_SST_MIRANDA_EVENT
#define _H_SST_MIRANDA_EVENT

#include <cstdint>
#include <deque>
#include <string>
#include <utility>

#include <sst/core/event.h>
#include <sst/core/params.h>

namespace SST {
namespace Miranda {

//==============================================================================
// MirandaReqEvent
//==============================================================================
class MirandaReqEvent : public SST::Event {
public:
    // 1) default ctor for deserialization / checkpoint-restart
    MirandaReqEvent() = default;

    // 2) “normal” ctor you probably already had
    MirandaReqEvent(uint64_t _key,
                    const std::deque<std::pair<std::string,SST::Params>>& gens) :
        key(_key), generators(gens)
    {}

    // data members
    uint64_t key;
    std::deque<std::pair<std::string,SST::Params>> generators;

private:
    // serialize in declaration order, calling base class first
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        Event::serialize_order(ser);
        SST_SER(key);
        SST_SER(generators);
    }

    ImplementSerializable(SST::Miranda::MirandaReqEvent);
};

//==============================================================================
// MirandaRspEvent
//==============================================================================
class MirandaRspEvent : public SST::Event {
public:
    // default ctor for UNPACK
    MirandaRspEvent() = default;

    // convenience ctor
    explicit MirandaRspEvent(uint64_t _key) : key(_key) {}

    uint64_t key;

private:
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        Event::serialize_order(ser);
        SST_SER(key);
    }

    ImplementSerializable(SST::Miranda::MirandaRspEvent);
};

} // namespace Miranda
} // namespace SST

#endif // _H_SST_MIRANDA_EVENT
