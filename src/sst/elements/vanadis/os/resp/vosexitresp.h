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

#ifndef _H_VANADIS_EXIT_RESPONSE
#define _H_VANADIS_EXIT_RESPONSE

#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisExitResponse : public SST::Event {

public:
    VanadisExitResponse() : SST::Event(), hw_thr(-1), return_code(0) {}
    VanadisExitResponse(int64_t rc, int thr = -1) : SST::Event(), hw_thr(thr), return_code(rc) {}
    ~VanadisExitResponse() {}

    int64_t getReturnCode() const { return return_code; }
    void setHWThread( int thr ) { hw_thr = thr; }
    int getHWThread() const { assert(hw_thr>-1); return hw_thr; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& return_code;
        ser& hw_thr;
    }

    ImplementSerializable(SST::Vanadis::VanadisExitResponse);

    int64_t return_code;
    int hw_thr;
};

} // namespace Vanadis
} // namespace SST

#endif
