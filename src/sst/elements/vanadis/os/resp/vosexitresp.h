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

#ifndef _H_VANADIS_EXIT_RESPONSE
#define _H_VANADIS_EXIT_RESPONSE

#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisExitResponse : public SST::Event {

public:
    VanadisExitResponse() : SST::Event(), return_code(0) {}
    VanadisExitResponse(int64_t rc) : SST::Event(), return_code(rc) {}
    ~VanadisExitResponse() {}

    int64_t getReturnCode() const { return return_code; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& return_code;
    }

    ImplementSerializable(SST::Vanadis::VanadisExitResponse);

    int64_t return_code;
};

} // namespace Vanadis
} // namespace SST

#endif
