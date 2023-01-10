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

class _VanadisStartThreadBaseReq : public SST::Event {
public:
    _VanadisStartThreadBaseReq() : SST::Event(), thread(0), instPtr(0), stackAddr(0), argAddr(0), tlsAddr(0) { }

    _VanadisStartThreadBaseReq( int thread, uint64_t instPtr, uint64_t stackAddr, uint64_t argAddr, uint64_t tlsAddr ) : 
        SST::Event(), thread(thread), stackAddr(stackAddr), instPtr(instPtr), argAddr(argAddr), tlsAddr(tlsAddr) {}

    virtual ~_VanadisStartThreadBaseReq() {}

    int     getThread() { return thread; }
    int64_t getInstPtr() { return instPtr; }
    int64_t getStackAddr() { return stackAddr; }
    int64_t getArgAddr() { return argAddr; }
    int64_t getTlsAddr() { return tlsAddr; }
    void setIntRegs( std::vector<uint64_t>& regs ) { intRegs = regs; } 
    void setFpRegs( std::vector<uint64_t>& regs ) { fpRegs = regs; } 
    std::vector<uint64_t>& getIntRegs() { return intRegs; }
    std::vector<uint64_t>& getFpRegs() { return fpRegs; }


private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& thread;
        ser& instPtr;
        ser& stackAddr;
        ser& argAddr;
        ser& tlsAddr;
        ser& intRegs;
        ser& fpRegs;
    }

    ImplementSerializable(SST::Vanadis::_VanadisStartThreadBaseReq);

    int     thread;
    int64_t instPtr;
    int64_t stackAddr;
    int64_t argAddr;
    int64_t tlsAddr;

    std::vector<uint64_t> intRegs;
    std::vector<uint64_t> fpRegs;
};

class VanadisStartThreadFirstReq : public _VanadisStartThreadBaseReq {
public:
    VanadisStartThreadFirstReq() : _VanadisStartThreadBaseReq() {}

    VanadisStartThreadFirstReq( int thread, uint64_t instPtr, uint64_t stackAddr ) : 
        _VanadisStartThreadBaseReq( thread, instPtr, stackAddr, 0, 0 ) {} 
};

class VanadisStartThreadForkReq : public _VanadisStartThreadBaseReq {
public:
    VanadisStartThreadForkReq() : _VanadisStartThreadBaseReq() {}

    VanadisStartThreadForkReq( int thread, uint64_t instPtr, uint64_t tlsAddr ) : 
        _VanadisStartThreadBaseReq( thread, instPtr, 0, 0, tlsAddr ) {} 
};

class VanadisStartThreadCloneReq : public _VanadisStartThreadBaseReq {
public:
    VanadisStartThreadCloneReq() : _VanadisStartThreadBaseReq() {}

    VanadisStartThreadCloneReq( int thread, uint64_t instPtr, uint64_t stackAddr, uint64_t tlsAddr, uint64_t argAddr ) : 
        _VanadisStartThreadBaseReq( thread, instPtr, stackAddr, argAddr, tlsAddr ) {} 
};


#if 0
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

    std::vector<uint64_t> intRegs;
    std::vector<uint64_t> fpRegs;

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
#endif

} // namespace Vanadis
} // namespace SST

#endif
