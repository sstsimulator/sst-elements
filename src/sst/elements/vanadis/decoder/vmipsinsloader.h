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

#ifndef _H_VANADIS_MIPS_INST_LOADER
#define _H_VANADIS_MIPS_INST_LOADER

#include "decoder/vinsloader.h"

namespace SST {
namespace Vanadis {

class VanadisMIPSInstructionLoader : public VanadisInstructionLoader {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisMIPSInstructionLoader)

    VanadisMIPSInstructionLoader(ComponentId_t& id, Params& params) : VanadisInstructionLoader(id, params) {}

    void hasEntryForAddress(const uint64_t ip) const { return instr_map.find(ip) != instr_map.end(); }

protected:
};

} // namespace Vanadis
} // namespace SST

#endif
