// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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

class VanadisStoreConditionalInstruction : public virtual VanadisStoreInstruction
{
public:
    VanadisStoreConditionalInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t memAddrReg,
        const int64_t offset, const uint16_t valueReg, const uint16_t condResultReg, const uint16_t store_width,
        VanadisStoreRegisterType reg_type) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 
            reg_type == STORE_INT_REGISTER ? 2 : 1, 1,
            reg_type == STORE_INT_REGISTER ? 2 : 1, 1,
            reg_type == STORE_FP_REGISTER ? 1 : 0, 0,
            reg_type == STORE_FP_REGISTER ? 1 : 0, 0),
        VanadisStoreInstruction(
            addr, hw_thr, isa_opts, memAddrReg, offset, valueReg, store_width, MEM_TRANSACTION_LLSC_STORE, reg_type),
            value_success(1), value_failure(0)
    {
        isa_int_regs_out[0] = condResultReg;
    }

    VanadisStoreConditionalInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t memAddrReg,
        const int64_t offset, const uint16_t valueReg, const uint16_t condResultReg, const uint16_t store_width,
        VanadisStoreRegisterType reg_type, int64_t successValue, int64_t failureValue) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 
            reg_type == STORE_INT_REGISTER ? 2 : 1, 1,
            reg_type == STORE_INT_REGISTER ? 2 : 1, 1,
            reg_type == STORE_FP_REGISTER ? 1 : 0, 0,
            reg_type == STORE_FP_REGISTER ? 1 : 0, 0),
        VanadisStoreInstruction(
            addr, hw_thr, isa_opts, memAddrReg, offset, valueReg, store_width, MEM_TRANSACTION_LLSC_STORE, reg_type),
            value_success(successValue), value_failure(failureValue)
    {
        isa_int_regs_out[0] = condResultReg;
    }

    VanadisStoreConditionalInstruction(const VanadisStoreConditionalInstruction& copy_me) :
        VanadisInstruction(copy_me),VanadisStoreInstruction(copy_me), value_success(copy_me.value_success),
        value_failure(copy_me.value_failure) 
        {
            ;
        }

    VanadisStoreConditionalInstruction* clone() { 
        return new VanadisStoreConditionalInstruction(*this); 
    }

    int64_t getResultSuccess() const { return value_success; }
    int64_t getResultFailure() const { return value_failure; }

protected: 
    const int64_t value_success;
    const int64_t value_failure;

};

} // namespace Vanadis
} // namespace SST

#endif
