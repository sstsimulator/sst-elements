#pragma once 

#include "emberNetworkIOEvent.h"

namespace SST {
namespace Ember {

class EmberNetworkIOWriteEvent : public EmberNetworkIOEvent {
public:
    EmberNetworkIOWriteEvent(NetworkIO::Interface& api, Output* output, uint64_t offset,
                           Hermes::MemAddr src, uint32_t length,
                           EmberEventTimeStatistic* stat = NULL) :
        EmberNetworkIOEvent(api, output, stat),
        m_offset(offset), m_src(src), m_length(length)
    {}

    ~EmberNetworkIOWriteEvent() {}

    std::string getName() { return "NetworkIOWrite"; }

    virtual void issue(uint64_t time, Callback callback) {
        EmberEvent::issue(time);
        m_api.networkIOWrite(m_offset, m_src.getSimVAddr(), m_length, callback);
    }

private:
    uint64_t m_offset;
    Hermes::MemAddr m_src;
    uint32_t m_length;
};

}
}
