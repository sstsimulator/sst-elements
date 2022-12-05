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

#ifndef VECSHIFTREGISTER_H_
#define VECSHIFTREGISTER_H_

#include <iostream>
#include <inttypes.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <uint.h>
#include <sint.h>
#include <sst/core/sst_config.h>
#include "sst_config.h"
namespace SST {
//#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)
typedef struct Rtlheader {
  UInt<4> delays_0;
  UInt<4> delays_1;
  UInt<4> delays_2;
  UInt<4> delays_3;
  UInt<1> clock;
  UInt<1> reset;
  UInt<4> io_ins_0;
  UInt<4> io_ins_1;
  UInt<4> io_ins_2;
  UInt<4> io_ins_3;
  UInt<1> io_load;
  UInt<1> io_shift;
  UInt<4> io_out;


  Rtlheader() {
    delays_0.rand_init();
    delays_1.rand_init();
    delays_2.rand_init();
    delays_3.rand_init();
    reset.rand_init();
    io_ins_0.rand_init();
    io_ins_1.rand_init();
    io_ins_2.rand_init();
    io_ins_3.rand_init();
    io_load.rand_init();
    io_shift.rand_init();
    io_out.rand_init();
  }

  void eval(bool update_registers, bool verbose, bool done_reset) {
    UInt<4> _GEN_0 = io_shift ? io_ins_0 : delays_0;
    UInt<4> _GEN_1 = io_shift ? delays_0 : delays_1;
    UInt<4> _GEN_2 = io_shift ? delays_1 : delays_2;
    UInt<4> _GEN_3 = io_shift ? delays_2 : delays_3;
    io_out = delays_3;
    if(update_registers)
        printf("\nio_out: %" PRIu8, io_out);
    if (update_registers) delays_0 = io_load ? io_ins_0 : _GEN_0;
    if (update_registers) delays_1 = io_load ? io_ins_1 : _GEN_1;
    if (update_registers) delays_2 = io_load ? io_ins_2 : _GEN_2;
    if (update_registers) delays_3 = io_load ? io_ins_3 : _GEN_3;
  }
} Rtlheader;
} //namespace SST
#endif  // VECSHIFTREGISTER_H_
