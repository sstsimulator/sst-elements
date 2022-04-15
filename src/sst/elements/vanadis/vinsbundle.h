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

#ifndef _H_VANADIS_INST_BUNDLE
#define _H_VANADIS_INST_BUNDLE

#include <cinttypes>
#include <cstdint>
#include <vector>

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisInstructionBundle {

public:
    VanadisInstructionBundle(const uint64_t addr) : ins_addr(addr), pc_inc(4) { inst_bundle.reserve(1); }

    ~VanadisInstructionBundle() { clear(); }

    void clear() {
        for (VanadisInstruction* next_ins : inst_bundle) {
            delete next_ins;
        }

        inst_bundle.clear();
    }

    uint32_t getInstructionCount() const { return inst_bundle.size(); }

    void addInstruction(VanadisInstruction* newIns) {
        inst_bundle.push_back(newIns->clone());
    }

    VanadisInstruction* getInstructionByIndex(const uint32_t index) {
        return inst_bundle[index];
    }

    uint64_t getInstructionAddress() const { return ins_addr; }
	 uint64_t pcIncrement() const { return pc_inc; }
	 void setPCIncrement(uint64_t newPCInc) { pc_inc = newPCInc; }

private:
    const uint64_t ins_addr;
	 uint64_t pc_inc;
    std::vector<VanadisInstruction*> inst_bundle;
};

} // namespace Vanadis
} // namespace SST

#endif
