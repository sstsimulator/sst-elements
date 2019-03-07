
#ifndef _H_SHOGUN_INIT_EVENT
#define _H_SHOGUN_INIT_EVENT

#include <sst/core/event.h>

namespace SST {
namespace Shogun {

    class ShogunInitEvent : public SST::Event {

    public:
        ShogunInitEvent()
            : port_count(01)
            , netID(-1)
            , queue_slots(-1)
        {
        }

        ShogunInitEvent(
            const int portCnt,
            const int netid,
            const int qSlot)
            : port_count(portCnt)
            , netID(netid)
            , queue_slots(qSlot)
        {
        }

        int getPortCount() const
        {
            return port_count;
        }

        int getNetworkID() const
        {
            return netID;
        }

        int getQueueSlotCount() const
        {
            return queue_slots;
        }

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            Event::serialize_order(ser);
            ser& port_count;
            ser& netID;
            ser& queue_slots;
        }

        ImplementSerializable(SST::Shogun::ShogunInitEvent);

    private:
        int port_count;
        int netID;
        int queue_slots;
    };

}
}

#endif
