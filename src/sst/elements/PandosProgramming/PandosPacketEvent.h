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
};

/**
 * A PANDO Request Packet
 */
class PandosRequestEventT : public PandosPacketEventT {
public:
        PandosRequestEventT():PandosPacketEventT() {}
        virtual ~PandosRequestEventT() {}

        int64_t src_pxn;
        int64_t src_core;
        size_t  size;
        pando::backend::address_t dst;
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

        std::vector<unsigned char> payload;
        
        ImplementSerializable(SST::PandosProgramming::PandosWriteRequestEventT);
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
        ImplementSerializable(SST::PandosProgramming::PandosReadResponseEventT);
};

/**
 * Response Packet to a Write Request
 */
class PandosWriteResponseEventT : public PandosResponseEventT {
public:        
        PandosWriteResponseEventT():PandosResponseEventT() {}
        virtual ~PandosWriteResponseEventT() {}
        ImplementSerializable(SST::PandosProgramming::PandosWriteResponseEventT);
};
                
}
}
