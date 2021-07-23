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

#ifndef _H_VANADIS_SYSCALL_RESPONSE
#define _H_VANADIS_SYSCALL_RESPONSE

#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisSyscallResponse : public SST::Event {
public:
    VanadisSyscallResponse() : SST::Event(), return_code(0), mark_success(true) {}
    VanadisSyscallResponse(int64_t ret_c) : SST::Event(), return_code(ret_c) { mark_success = true; }
    ~VanadisSyscallResponse() {}

    int64_t getReturnCode() const { return return_code; }
    bool isSuccessful() const { return mark_success; }
    void markFailed() { mark_success = false; }
    void markSuccessful() { mark_success = true; }
    void setSuccess(const bool succ) { mark_success = succ; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        ser& return_code;
        ser& mark_success;
    }

    ImplementSerializable(SST::Vanadis::VanadisSyscallResponse);
    int64_t return_code;
    bool mark_success;
};

} // namespace Vanadis
} // namespace SST

#endif
