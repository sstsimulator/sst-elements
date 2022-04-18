// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_SYSCALL_CLOSE
#define _H_VANADIS_SYSCALL_CLOSE

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallCloseEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallCloseEvent() : VanadisSyscallEvent() {}
    VanadisSyscallCloseEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, int32_t file_id)
        : VanadisSyscallEvent(core, thr, bittype), close_file_descriptor(file_id) {}

    VanadisSyscallOp getOperation() { return SYSCALL_OP_CLOSE; }

    int32_t getFileDescriptor() const { return close_file_descriptor; }

private:
    int32_t close_file_descriptor;
};

} // namespace Vanadis
} // namespace SST

#endif
