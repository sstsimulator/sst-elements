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

class VanadisGetThreadStateReq : public SST::Event {
public:
    VanadisGetThreadStateReq() : SST::Event(), thread(-1) {}

    VanadisGetThreadStateReq( int thread) : SST::Event(), thread(thread) {}

    ~VanadisGetThreadStateReq() {}

    int     getThread() { return thread; }
private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& thread;
    }

    ImplementSerializable(SST::Vanadis::VanadisGetThreadStateReq);

    int     thread;
};

class VanadisGetThreadStateResp : public SST::Event {
public:
    VanadisGetThreadStateResp() : SST::Event(), core(-1), thread(-1), instPtr(-1), tlsPtr(-1) {}

    VanadisGetThreadStateResp( int core, int thread, uint64_t instPtr, uint64_t tlsPtr) : SST::Event(), core(core), thread(thread), instPtr(instPtr), tlsPtr(tlsPtr) {}

    ~VanadisGetThreadStateResp() {}

    int     getThread() { return thread; }
    int     getCore() { return core; }

    uint64_t getInstPtr() { return instPtr; }
    uint64_t getTlsPtr() { return tlsPtr; }

    std::vector<uint64_t> intRegs;
    std::vector<uint64_t> fpRegs;

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& thread;
        ser& core;
        ser& intRegs;
        ser& fpRegs;
        ser& instPtr;
        ser& tlsPtr;
    }

    ImplementSerializable(SST::Vanadis::VanadisGetThreadStateResp);

    int     thread;
    int     core;
    uint64_t instPtr;
    uint64_t tlsPtr;
};

} // namespace Vanadis
} // namespace SST

#endif
