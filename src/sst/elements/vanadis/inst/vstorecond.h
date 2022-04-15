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

#ifndef _H_VANADIS_STORE_CONDITIONAL
#define _H_VANADIS_STORE_CONDITIONAL

#include "inst/vstore.h"

namespace SST {
namespace Vanadis {

class VanadisStoreConditionalInstruction : public VanadisStoreInstruction
{

public:
    VanadisStoreConditionalInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t memAddrReg,
        const int64_t offset, const uint16_t valueReg, const uint16_t condResultReg, const uint16_t store_width,
        VanadisStoreRegisterType reg_type) :
        VanadisStoreInstruction(
            addr, hw_thr, isa_opts, memAddrReg, offset, valueReg, store_width, MEM_TRANSACTION_LLSC_STORE, reg_type)
    {

        isa_int_regs_out[0] = condResultReg;
    }
};

} // namespace Vanadis
} // namespace SST

#endif
