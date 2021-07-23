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

#ifndef _H_VANADIS_MOVI
#define _H_VANADIS_MOVI

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisMoveCompareImmInstruction : public VanadisInstruction {
public:
  VanadisMoveCompareImmInstruction(const uint64_t addr, const uint32_t hw_thr,
                                   const VanadisDecoderOptions *isa_opts,
                                   const uint16_t dest, const uint16_t src,
                                   const uint16_t reg_comp,
                                   const uint64_t immediate,
                                   VanadisRegisterCompareType c_type)
      : VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0),
        imm_value(immediate), cmp_type(c_type) {

    isa_int_regs_in[0] = src;
    isa_int_regs_in[1] = reg_comp;
    isa_int_regs_out[0] = dest;
  }

  VanadisMoveCompareImmInstruction *clone() override {
    return new VanadisMoveCompareImmInstruction(*this);
  }

  VanadisFunctionalUnitType getInstFuncType() const override {
    return INST_INT_ARITH;
  }

  const char *getInstCode() const override {
    switch (cmp_type) {
    case REG_COMPARE_EQ:
      return "MOVEQ";
    case REG_COMPARE_NEQ:
      return "MOVNEQ";
    case REG_COMPARE_LT:
      return "MOVLT";
    case REG_COMPARE_LTE:
      return "MOVLTE";
    case REG_COMPARE_GT:
      return "MOVGT";
    case REG_COMPARE_GTE:
      return "MOVGTE";
    default:
      return "UNKNOWN";
    }
  }

  void printToBuffer(char *buffer, size_t buffer_size) override {
    snprintf(buffer, buffer_size,
             "%8s %5" PRIu16 " <- %5" PRIu16 " (compare-reg: %5" PRIu16
             " imm: %" PRIu64 ") (phys: %5" PRIu16 " <- %5" PRIu16
             " compare-reg: %5" PRIu16 ")",
             getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0],
             isa_int_regs_in[1], imm_value, phys_int_regs_out[0],
             phys_int_regs_in[0], phys_int_regs_in[1]);
  }

  void execute(SST::Output *output, VanadisRegisterFile *regFile) override {
#ifdef VANADIS_BUILD_DEBUG
    output->verbose(CALL_INFO, 16, 0,
                    "%8s %5" PRIu16 " <- %5" PRIu16 " (compare-reg: %5" PRIu16
                    " imm: %" PRIu64 ") (phys: %5" PRIu16 " <- %5" PRIu16
                    " compare-reg: %5" PRIu16 ")",
                    getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0],
                    isa_int_regs_in[1], imm_value, phys_int_regs_out[0],
                    phys_int_regs_in[0], phys_int_regs_in[1]);
#endif
    const uint64_t reg_value =
        regFile->getIntReg<uint64_t>(phys_int_regs_in[0]);
    const uint64_t compare_check =
        regFile->getIntReg<uint64_t>(phys_int_regs_in[1]);
    bool compare_result = false;

    switch (cmp_type) {
    case REG_COMPARE_EQ:
      compare_result = (compare_check == imm_value);
      break;
    case REG_COMPARE_NEQ:
      compare_result = (compare_check != imm_value);
    case REG_COMPARE_LT:
      compare_result = (compare_check < imm_value);
      break;
    case REG_COMPARE_LTE:
      compare_result = (compare_check <= imm_value);
      break;
    case REG_COMPARE_GT:
      compare_result = (compare_check > imm_value);
      break;
    case REG_COMPARE_GTE:
      compare_result = (compare_check >= imm_value);
      break;
    }

    if (compare_result) {
      regFile->setIntReg<uint64_t>(phys_int_regs_out[0], reg_value);
    }

    markExecuted();
  }

private:
  VanadisRegisterCompareType cmp_type;
  const uint64_t imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
