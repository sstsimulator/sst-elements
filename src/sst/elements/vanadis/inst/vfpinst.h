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

#ifndef _H_VANADIS_FP_INSTRUCTION
#define _H_VANADIS_FP_INSTRUCTION

#include "inst/vinst.h"
#include "decoder/visaopts.h"
#include "inst/regfile.h"
#include "inst/vinsttype.h"
#include "inst/vregfmt.h"

#include <cstring>
#include <sst/core/output.h>

namespace SST {
namespace Vanadis {

class VanadisFloatingPointInstruction : public VanadisInstruction
{
public:
    VanadisFloatingPointInstruction(
        const uint64_t address, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        const uint16_t c_phys_int_reg_in, const uint16_t c_phys_int_reg_out, const uint16_t c_isa_int_reg_in,
        const uint16_t c_isa_int_reg_out, const uint16_t c_phys_fp_reg_in, const uint16_t c_phys_fp_reg_out,
        const uint16_t c_isa_fp_reg_in, const uint16_t c_isa_fp_reg_out) :
			VanadisInstruction(
            address, hw_thr, isa_opts, c_phys_int_reg_in, c_phys_int_reg_out, c_isa_int_reg_in, c_isa_int_reg_out,
            c_phys_fp_reg_in, c_phys_fp_reg_out, c_isa_fp_reg_in, c_isa_fp_reg_out)
    {}

};

} // namespace Vanadis
} // namespace SST

#endif
