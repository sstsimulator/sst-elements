// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_DUMP_REGS_REQ
#define _H_VANADIS_DUMP_REGS_REQ

#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisDumpRegsReq : public SST::Event {
public:
    VanadisDumpRegsReq() : SST::Event(), thread(0) { }

    VanadisDumpRegsReq( int thread) : 
        SST::Event(), thread(thread) {}

    ~VanadisDumpRegsReq() {}

    int     getThread() { return thread; }


private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& thread;
    }

    ImplementSerializable(SST::Vanadis::VanadisDumpRegsReq);

    int     thread;
};

} // namespace Vanadis
} // namespace SST

#endif
