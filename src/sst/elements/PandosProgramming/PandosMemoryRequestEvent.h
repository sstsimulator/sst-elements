#pragma once
#include <sst/core/event.h>
namespace SST {
namespace PandosProgramming {
class PandosMemoryRequestEventT : public SST::Event
{
public:
        // constructors
        PandosMemoryRequestEventT()
                :SST::Event()
                ,size(4)
                ,src_core(0){
        }

        // serialization function
        void serialize_order(SST::Core::Serialization::serializer &ser) override {
                Event::serialize_order(ser);
                ser & size;
                ser & src_core;
        }

        
        // members
        size_t size;
        int src_core;

        // register event as serializable
        ImplementSerializable(SST::PandosProgramming::PandosMemoryRequestEventT);
};
}
}
