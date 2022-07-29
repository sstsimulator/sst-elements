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

#ifndef _H_VANADIS_FP_INSTRUCTION
#define _H_VANADIS_FP_INSTRUCTION

#include "decoder/visaopts.h"
#include "inst/regfile.h"
#include "inst/vinst.h"
#include "inst/vinsttype.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"
#include "vfpflags.h"

#include <cstring>
#include <cmath>
#include <sst/core/output.h>

namespace SST {
namespace Vanadis {

class VanadisFloatingPointInstruction : public VanadisInstruction
{
public:
    VanadisFloatingPointInstruction(
        const uint64_t address, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fp_flags, const uint16_t c_phys_int_reg_in, const uint16_t c_phys_int_reg_out,
        const uint16_t c_isa_int_reg_in, const uint16_t c_isa_int_reg_out, const uint16_t c_phys_fp_reg_in,
        const uint16_t c_phys_fp_reg_out, const uint16_t c_isa_fp_reg_in, const uint16_t c_isa_fp_reg_out) :
        VanadisInstruction(
            address, hw_thr, isa_opts, c_phys_int_reg_in, c_phys_int_reg_out, c_isa_int_reg_in, c_isa_int_reg_out,
            c_phys_fp_reg_in, c_phys_fp_reg_out, c_isa_fp_reg_in, c_isa_fp_reg_out),
        pipeline_fpflags(fp_flags),
        update_fp_flags(false)
    {}

    VanadisFloatingPointInstruction(const VanadisFloatingPointInstruction& copy_me) :
        VanadisInstruction(copy_me),
        update_fp_flags(copy_me.update_fp_flags),
        fpflags(copy_me.fpflags),
		  pipeline_fpflags(copy_me.pipeline_fpflags)
    {}

    virtual bool updatesFPFlags() const { return update_fp_flags; }
    virtual void performFPFlagsUpdate() const {
		pipeline_fpflags->copy(fpflags);
	 }

protected:
    bool                       update_fp_flags;
	 VanadisFloatingPointFlags* pipeline_fpflags;
    VanadisFloatingPointFlags  fpflags;

	 template<typename T>
    void performFlagChecks(const T value) {
		if(std::fpclassify(value) == FP_INFINITE) {
			fpflags.setOverflow();
			update_fp_flags = true;
		}

		if(std::fpclassify(value) == FP_SUBNORMAL) {
			fpflags.setUnderflow();
			update_fp_flags = true;
		}
	 }
};

} // namespace Vanadis
} // namespace SST

#endif
