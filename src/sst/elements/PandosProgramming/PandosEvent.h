#pragma once

#include <sst/core/event.h>

namespace SST {
namespace PandosProgramming {

class PandosEventT : public SST::Event
{
public:
        // constructor
        PandosEventT() : SST::Event() {}

        // example data members
        std::vector<char> payload;

        // serializing function
        void serialize_order(SST::Core::Serialization::serializer &ser) override {
                Event::serialize_order(ser);
                ser & payload;
        }

        // register event as serializable
        ImplementSerializable(SST::PandosProgramming::PandosEventT);
};

}
}
