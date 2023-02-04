// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_GET_THREAD_STATE
#define _H_VANADIS_GET_THREAD_STATE

#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisCoreEvent : public SST::Event {
public:
    VanadisCoreEvent() : SST::Event(), thread(-1), core(-1) {}
    VanadisCoreEvent( int thread ) : SST::Event(), core(-1), thread(thread) {}
    VanadisCoreEvent( int core, int thread ) : SST::Event(), core(core), thread(thread) {}
    int     getThread() { return thread; }
    int     getCore() { return core; }
private:
    int     thread;
    int     core;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& thread;
        ser& core;
    }

    ImplementSerializable(SST::Vanadis::VanadisCoreEvent);
};

class VanadisGetThreadStateReq : public VanadisCoreEvent {
public:
    VanadisGetThreadStateReq() : VanadisCoreEvent() {}

    VanadisGetThreadStateReq( int thread) : VanadisCoreEvent( thread ) {}

    ~VanadisGetThreadStateReq() {}
};

class VanadisGetThreadStateResp : public VanadisCoreEvent {
public:
    VanadisGetThreadStateResp() : VanadisCoreEvent(), instPtr(-1), tlsPtr(-1) {}

    VanadisGetThreadStateResp( int core, int thread, uint64_t instPtr, uint64_t tlsPtr) : VanadisCoreEvent( core, thread), instPtr(instPtr), tlsPtr(tlsPtr) {}

    ~VanadisGetThreadStateResp() {}

    uint64_t getInstPtr() { return instPtr; }
    uint64_t getTlsPtr() { return tlsPtr; }

    std::vector<uint64_t> intRegs;
    std::vector<uint64_t> fpRegs;

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& intRegs;
        ser& fpRegs;
        ser& instPtr;
        ser& tlsPtr;
    }

    ImplementSerializable(SST::Vanadis::VanadisGetThreadStateResp);

    uint64_t instPtr;
    uint64_t tlsPtr;
};

} // namespace Vanadis
} // namespace SST

#endif
