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

#ifndef _H_VANADIS_START_THREAD_REQ
#define _H_VANADIS_START_THREAD_REQ

#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisStartThreadReq : public SST::Event {
public:
    VanadisStartThreadReq() : SST::Event(), thread(0), stackStart(0), instructionPointer(0), startArg(0) { }

    VanadisStartThreadReq( int thread, uint64_t stackStart, uint64_t instructionPointer, uint64_t startArg = 0, uint64_t tlsAddr = 0 ) : 
        SST::Event(), thread(thread), stackStart(stackStart), instructionPointer(instructionPointer), startArg(startArg), tlsAddr(tlsAddr) {}

    ~VanadisStartThreadReq() {}

    int     getThread() { return thread; }
    int64_t getStackStart() { return stackStart; }
    int64_t getInstructionPointer() { return instructionPointer; }
    int64_t getStartArg() { return startArg; }
    int64_t getTlsAddr() { return tlsAddr; }


private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& thread;
        ser& stackStart;
        ser& instructionPointer;
        ser& startArg;
        ser& tlsAddr;
    }

    ImplementSerializable(SST::Vanadis::VanadisStartThreadReq);

    int     thread;
    int64_t stackStart;
    int64_t instructionPointer;
    int64_t startArg;
    int64_t tlsAddr;
};

class VanadisStartThreadFullReq : public SST::Event {
public:
    VanadisStartThreadFullReq() : SST::Event(), core(-1), thread(-1), instPtr(-1), tlsPtr(-1) {}

    VanadisStartThreadFullReq( int core, int thread, uint64_t instPtr, uint64_t tlsPtr ) : SST::Event(), core(core), thread(thread), instPtr(instPtr), tlsPtr(tlsPtr) {}

    ~VanadisStartThreadFullReq() {}

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

    ImplementSerializable(SST::Vanadis::VanadisStartThreadFullReq);

    int     thread;
    int     core;
    uint64_t instPtr;
    uint64_t tlsPtr;
};


} // namespace Vanadis
} // namespace SST

#endif
