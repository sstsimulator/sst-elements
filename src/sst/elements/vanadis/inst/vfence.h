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

#ifndef _H_VANADIS_FENCE_INST
#define _H_VANADIS_FENCE_INST

#include "inst/vinst.h"

#include <cstdio>
#include <cstdint>

namespace SST {
namespace Vanadis {

enum VanadisFenceType { VANADIS_LOAD_FENCE, VANADIS_STORE_FENCE, VANADIS_LOAD_STORE_FENCE };

class VanadisFenceInstruction : public virtual VanadisInstruction
{
public:
    VanadisFenceInstruction(
        const uint64_t address, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        const VanadisFenceType fenceT) :
        VanadisInstruction(address, hw_thr, isa_opts, 0, 0, 0, 0, 0, 0, 0, 0)
    {
        fence = fenceT;
    }

    VanadisFenceInstruction* clone() override { return new VanadisFenceInstruction(*this); }

    bool createsLoadFence() const { return (fence == VANADIS_LOAD_FENCE) || (fence == VANADIS_LOAD_STORE_FENCE); }

    bool createsStoreFence() const { return (fence == VANADIS_STORE_FENCE) || (fence == VANADIS_LOAD_STORE_FENCE); }

    virtual const char* getInstCode() const override
    {
        return (fence == VANADIS_LOAD_STORE_FENCE) ? "SYNC" : (fence == VANADIS_LOAD_FENCE) ? "LFENCE" : "SFENCE";
    }

    virtual void printToBuffer(char* buffer, size_t buffer_size) override { snprintf(buffer, buffer_size, "%s", getInstCode()); }

    virtual VanadisFunctionalUnitType getInstFuncType() const override { return INST_FENCE; }

    virtual void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override {}

    virtual void print(SST::Output* output) override { output->verbose(CALL_INFO, 8, 0, "%s", getInstCode()); }

protected:
    VanadisFenceType fence;
};



} // namespace Vanadis
} // namespace SST

#endif
