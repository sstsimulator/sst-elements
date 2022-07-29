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

#ifndef _H_VANADIS_INSTRUCTION_DECODE_FAULT
#define _H_VANADIS_INSTRUCTION_DECODE_FAULT

#include "decoder/visaopts.h"
#include "inst/vfault.h"
#include "inst/vinsttype.h"

namespace SST {
namespace Vanadis {

class VanadisInstructionDecodeFault : public VanadisInstructionFault
{
public:
    VanadisInstructionDecodeFault(
        const uint64_t address, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts) :
        VanadisInstructionFault(address, hw_thr, isa_opts)
    {}

    VanadisInstruction* clone() override
    {
        return new VanadisInstructionDecodeFault(ins_address, hw_thread, isa_options);
    }

    const char* getInstCode() const override { return "DECODE_FAULT"; }
};

} // namespace Vanadis
} // namespace SST

#endif
