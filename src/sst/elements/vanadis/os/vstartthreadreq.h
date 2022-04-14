// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
    VanadisStartThreadReq() : SST::Event(), thread(0), stackStart(0), instructionPointer(0) { }

    VanadisStartThreadReq( int thread, uint64_t stackStart, uint64_t instructionPointer) : 
        SST::Event(), thread(thread), stackStart(stackStart), instructionPointer(instructionPointer) {}

    ~VanadisStartThreadReq() {}

    int     getThread() { return thread; }
    int64_t getStackStart() { return stackStart; }
    int64_t getInstructionPointer() { return instructionPointer; }
private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& thread;
        ser& stackStart;
        ser& instructionPointer;
    }

    ImplementSerializable(SST::Vanadis::VanadisStartThreadReq);

    int     thread;
    int64_t stackStart;
    int64_t instructionPointer;
};

} // namespace Vanadis
} // namespace SST

#endif
