
#ifndef _H_SHOGUN_INC_CREDIT_EVENT
#define _H_SHOGUN_INC_CREDIT_EVENT

#include <sst/core/event.h>

namespace SST {
namespace Shogun {

    class ShogunCreditEvent : public SST::Event {

    public:
        ShogunCreditEvent()
            : sourcePort(0)
        {
        }
        ShogunCreditEvent(const int source)
            : sourcePort(source)
        {
        }
        ~ShogunCreditEvent() {}

        int getSrc() const
        {
            return sourcePort;
        }

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            Event::serialize_order(ser);

            ser& sourcePort;
        }

        ImplementSerializable(SST::Shogun::ShogunCreditEvent);

    protected:
        int sourcePort;
    };

}
}

#endif
