#pragma once
#include <sst/core/event.h>
#include <pando/backend_address.hpp>
namespace SST {
namespace PandosProgramming {
/**
 * A PANDO Packet - Sent over links.
 */
class PandosPacketEventT : public SST::Event
{
public:
        PandosPacketEventT():SST::Event(){}
        virtual ~PandosPacketEventT() {}
        
        ImplementSerializable(SST::PandosProgramming::PandosPacketEventT);
};

/**
 * A PANDO Request Packet
 */
class PandosRequestEventT : public PandosPacketEventT {
public:
        PandosRequestEventT():PandosPacketEventT() {}
        virtual ~PandosRequestEventT() {}

        int64_t   src_pxn;
        int64_t   src_core;
        size_t    size;
        bool      dst_dram_not_spm;
        int64_t   dst_pxn;
        int64_t   dst_core;
        uintptr_t dst_uptr;
        
        pando::backend::address_t getDst() const {
                pando::backend::address_t a;
                a.setDRAMNotSPM(dst_dram_not_spm);
                a.setPXN(dst_pxn);
                a.setCore(dst_core);
                a.setAddress(dst_uptr);
                return a;
        }
        
        void setDst(const pando::backend::address_t &address) {
                dst_dram_not_spm = address.getDRAMNotSPM();
                dst_pxn = address.getPXN();
                dst_core = address.getCore();
                dst_uptr = address.getAddress();
        }
        
        // serialization function
        void serialize_order(SST::Core::Serialization::serializer &ser) override {
                PandosPacketEventT::serialize_order(ser);
                ser & src_pxn;
                ser & src_core;
                ser & size;
                ser & dst_dram_not_spm;
                ser & dst_pxn;
                ser & dst_core;
                ser & dst_uptr;
        }

        ImplementSerializable(SST::PandosProgramming::PandosRequestEventT);
};

/**
 * A PANDO Read Request Packet
 */
class PandosReadRequestEventT : public PandosRequestEventT {
public:
        PandosReadRequestEventT():PandosRequestEventT() {}
        virtual ~PandosReadRequestEventT() {}
        ImplementSerializable(SST::PandosProgramming::PandosReadRequestEventT);
};

/**
 * A PANDO Write Request Packet
 */
class PandosWriteRequestEventT : public PandosRequestEventT {
public:        
        PandosWriteRequestEventT():PandosRequestEventT() {}
        virtual ~PandosWriteRequestEventT() {}

        // serialization function
        void serialize_order(SST::Core::Serialization::serializer &ser) override {
                PandosRequestEventT::serialize_order(ser);
                ser & payload;
        }

        std::vector<unsigned char> payload;
        
        ImplementSerializable(SST::PandosProgramming::PandosWriteRequestEventT);
};

/**
 * A PANDO Atomic Integer Add Packet
 */
class PandosAtomicIAddRequestEventT : public PandosRequestEventT {
public:
    PandosAtomicIAddRequestEventT():PandosRequestEventT(){}
    virtual ~PandosAtomicIAddRequestEventT(){}

    // serialization function
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        PandosRequestEventT::serialize_order(ser);
        ser & payload;
    }

    std::vector<unsigned char> payload;

    ImplementSerializable(SST::PandosProgramming::PandosAtomicIAddRequestEventT);
};

/**
 * A PANDO Delegate Request Packet
 */
class PandosDelegateRequestEventT : public PandosRequestEventT {
public:
        PandosDelegateRequestEventT():PandosRequestEventT(){}
        virtual ~PandosDelegateRequestEventT() {}

        // serialization function
        void serialize_order(SST::Core::Serialization::serializer &ser) override {
                PandosRequestEventT::serialize_order(ser);
                ser & task_type_id;
                ser & task_data;
        }

        ImplementSerializable(SST::PandosProgramming::PandosDelegateRequestEventT);
public:
        pando::backend::TaskTypeID_t task_type_id;
        std::vector<unsigned char> task_data;
};


/**
 * A PANDO Response Packet
 */
class PandosResponseEventT : public PandosPacketEventT {
public:
        PandosResponseEventT():PandosPacketEventT() {}
        virtual ~PandosResponseEventT() {}
        int64_t src_pxn;
        int64_t src_core;

        // serialization function
        void serialize_order(SST::Core::Serialization::serializer &ser) override {
                PandosPacketEventT::serialize_order(ser);
                ser & src_pxn;
                ser & src_core;
        }

        ImplementSerializable(SST::PandosProgramming::PandosResponseEventT);
};

/**
 * Response Packet to a Read Request
 */
class PandosReadResponseEventT : public PandosResponseEventT {
public:        
        PandosReadResponseEventT():PandosResponseEventT() {}
        virtual ~PandosReadResponseEventT() {}

        int64_t size;
        std::vector<unsigned char> payload;
        
        // serialization function
        void serialize_order(SST::Core::Serialization::serializer &ser) override {
                PandosResponseEventT::serialize_order(ser);
                ser & size;
                ser & payload;
        }
        
        ImplementSerializable(SST::PandosProgramming::PandosReadResponseEventT);
};

/**
 * Response Packet to a Write Request
 */
class PandosWriteResponseEventT : public PandosResponseEventT {
public:        
        PandosWriteResponseEventT():PandosResponseEventT() {}
        virtual ~PandosWriteResponseEventT() {}
        // register event as serializable
        ImplementSerializable(SST::PandosProgramming::PandosWriteResponseEventT);
};

/**
 * Response Packet to an Atomic Add
 */
class PandosAtomicIAddResponseEventT : public PandosResponseEventT {
public:
    PandosAtomicIAddResponseEventT():PandosResponseEventT(){}
    virtual ~PandosAtomicIAddResponseEventT() {}

    int64_t size;
    std::vector<unsigned char> payload;

    // serialization function
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        PandosResponseEventT::serialize_order(ser);
        ser & size;
        ser & payload;
    }

    // register event as serializable
    ImplementSerializable(SST::PandosProgramming::PandosAtomicIAddResponseEventT);
};

/**
 * A PANDO Delegate Response Packet
 */
class PandosDelegateResponseEventT : public PandosResponseEventT {
public:
        PandosDelegateResponseEventT():PandosResponseEventT(){}
        virtual ~PandosDelegateResponseEventT(){}
        // register event as serializable
        ImplementSerializable(SST::PandosProgramming::PandosDelegateResponseEventT);
};


}
}
