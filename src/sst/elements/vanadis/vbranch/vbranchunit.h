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

#ifndef _H_VANADIS_BRANCH_UNIT
#define _H_VANADIS_BRANCH_UNIT

#include <sst/core/subcomponent.h>

#include "inst/vspeculate.h"
#include <list>
#include <unordered_map>

namespace SST {
namespace Vanadis {

class VanadisBranchUnit : public SST::SubComponent {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisBranchUnit)

    SST_ELI_DOCUMENT_PARAMS()

    SST_ELI_DOCUMENT_STATISTICS()

    VanadisBranchUnit(ComponentId_t id, Params& params) : SubComponent(id) {}
    virtual ~VanadisBranchUnit() {}

    virtual void push(const uint64_t ins_addr, const uint64_t pred_addr) = 0;
    virtual uint64_t predictAddress(const uint64_t addr) = 0;
    virtual bool contains(const uint64_t addr) = 0;
};

} // namespace Vanadis
} // namespace SST

#endif
