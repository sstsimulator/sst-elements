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

#ifndef _H_VANADIS_ISA_OPTIONS
#define _H_VANADIS_ISA_OPTIONS

#include <cinttypes>
#include <cstdint>

#include "inst/fpregmode.h"

namespace SST {
namespace Vanadis {

class VanadisDecoderOptions {
public:
    VanadisDecoderOptions(const uint16_t reg_ignore, const uint16_t isa_int_reg_c, const uint16_t isa_fp_reg_c,
                          const uint16_t isa_sysc_reg, const VanadisFPRegisterMode fp_reg_m)
        : reg_ignore_writes(reg_ignore), isa_int_reg_count(isa_int_reg_c), isa_fp_reg_count(isa_fp_reg_c),
          isa_syscall_code_reg(isa_sysc_reg), fp_reg_mode(fp_reg_m) {}

    VanadisDecoderOptions()
        : reg_ignore_writes(0), isa_int_reg_count(0), isa_fp_reg_count(0), isa_syscall_code_reg(0),
          fp_reg_mode(VANADIS_REGISTER_MODE_FP32) {}

    uint16_t getRegisterIgnoreWrites() const { return reg_ignore_writes; }
    uint16_t countISAIntRegisters() const { return isa_int_reg_count; }
    uint16_t countISAFPRegisters() const { return isa_fp_reg_count; }
    uint16_t getISASysCallCodeReg() const { return isa_syscall_code_reg; }
    VanadisFPRegisterMode getFPRegisterMode() const { return fp_reg_mode; }

protected:
    const uint16_t reg_ignore_writes;
    const uint16_t isa_int_reg_count;
    const uint16_t isa_fp_reg_count;
    const uint16_t isa_syscall_code_reg;
    const VanadisFPRegisterMode fp_reg_mode;
};

} // namespace Vanadis
} // namespace SST

#endif
