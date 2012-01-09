/* zesto-decode.cpp - Zesto decode stage
 * 
 * Copyright © 2009 by Gabriel H. Loh and the Georgia Tech Research Corporation
 * Atlanta, GA  30332-0415
 * All Rights Reserved.
 * 
 * THIS IS A LEGAL DOCUMENT BY DOWNLOADING ZESTO, YOU ARE AGREEING TO THESE
 * TERMS AND CONDITIONS.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * NOTE: Portions of this release are directly derived from the SimpleScalar
 * Toolset (property of SimpleScalar LLC), and as such, those portions are
 * bound by the corresponding legal terms and conditions.  All source files
 * derived directly or in part from the SimpleScalar Toolset bear the original
 * user agreement.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the Georgia Tech Research Corporation nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * 4. Zesto is distributed freely for commercial and non-commercial use.  Note,
 * however, that the portions derived from the SimpleScalar Toolset are bound
 * by the terms and agreements set forth by SimpleScalar, LLC.  In particular:
 * 
 *   "Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 *   downloaded, compiled, executed, copied, and modified solely for nonprofit,
 *   educational, noncommercial research, and noncommercial scholarship
 *   purposes provided that this notice in its entirety accompanies all copies.
 *   Copies of the modified software can be delivered to persons who use it
 *   solely for nonprofit, educational, noncommercial research, and
 *   noncommercial scholarship purposes provided that this notice in its
 *   entirety accompanies all copies."
 * 
 * User is responsible for reading and adhering to the terms set forth by
 * SimpleScalar, LLC where appropriate.
 * 
 * 5. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 6. Noncommercial and nonprofit users may distribute copies of Zesto in
 * compiled or executable form as set forth in Section 2, provided that either:
 * (A) it is accompanied by the corresponding machine-readable source code, or
 * (B) it is accompanied by a written offer, with no time limit, to give anyone
 * a machine-readable copy of the corresponding source code in return for
 * reimbursement of the cost of distribution. This written offer must permit
 * verbatim duplication by anyone, or (C) it is distributed by someone who
 * received only the executable form, and is accompanied by a copy of the
 * written offer of source code.
 * 
 * 7. Zesto was developed by Gabriel H. Loh, Ph.D.  US Mail: 266 Ferst Drive,
 * Georgia Institute of Technology, Atlanta, GA 30332-0765
 */

#include "simplescalar/sim.h"
#include "simplescalar/thread.h"

#include "zesto-core.h"
#include "zesto-opts.h"
#include "zesto-oracle.h"
#include "zesto-bpred.h"
#include "zesto-fetch.h"
#include "zesto-decode.h"
#include "zesto-alloc.h"
#include "zesto-exec.h"
#include "zesto-commit.h"

void decode_reg_options(struct opt_odb_t * odb, struct core_knobs_t * knobs)
{

  /* decode pipe */
  opt_reg_int(odb, "-decode:depth","decode pipeline depth (stages) [DS]",
      &knobs->decode.depth, /*default*/ knobs->decode.depth, /*print*/true,/*format*/NULL);

  opt_reg_int(odb, "-decode:width","decode pipeline width (Macro-ops) [DS]",
      &knobs->decode.width, /*default*/ knobs->decode.width, /*print*/true,/*format*/NULL);

  opt_reg_int(odb, "-decode:targetstage","decode pipeline stage of branch address calculator [DS]",
      &knobs->decode.target_stage, /*default*/ knobs->decode.target_stage, /*print*/true,/*format*/NULL);

  opt_reg_int(odb, "-decode:branches","maximum branches decoded per cycle [D]",
      &knobs->decode.branch_decode_limit, /*default*/ knobs->decode.branch_decode_limit, /*print*/true,/*format*/NULL);

  /* actual decoders */
  opt_reg_int_list(odb, "-decoders", "maximum uops generated for each decoder (e.g., 4 1 1) [D]",
      knobs->decode.decoders, MAX_DECODE_WIDTH, &knobs->decode.num_decoder_specs, knobs->decode.decoders, /* print */true, /* format */NULL, /* !accrue */false);

  opt_reg_int(odb, "-MS:latency","additional latency for accessing ucode sequencer (cycles) [D]",
      &knobs->decode.MS_latency, /*default*/ knobs->decode.MS_latency, /*print*/true,/*format*/NULL);

  opt_reg_int(odb, "-uopQ:size","number of entries in uopQ [D]",
      &knobs->decode.uopQ_size, /*default*/ knobs->decode.uopQ_size, /*print*/true,/*format*/NULL);

  /* fusion options */
  opt_reg_flag(odb, "-fuse:none","disable all uop fusion rules [DS]",
      &knobs->decode.fusion_none, /*default*/ false, /*print*/true,/*format*/NULL);
  opt_reg_flag(odb, "-fuse:all","enable all uop fusion rules [D]",
      &knobs->decode.fusion_all, /*default*/ false, /*print*/true,/*format*/NULL);
  opt_reg_flag(odb, "-fuse:loadop","enable load-op uop fusion [D]",
      &knobs->decode.fusion_load_op, /*default*/ false, /*print*/true,/*format*/NULL);
  opt_reg_flag(odb, "-fuse:stastd","enable sta-std uop fusion [D]",
      &knobs->decode.fusion_sta_std, /*default*/ false, /*print*/true,/*format*/NULL);
  opt_reg_flag(odb, "-fuse:partial","enable uop-fusion of partial register write combining uops [D]",
      &knobs->decode.fusion_partial, /*default*/ false, /*print*/true,/*format*/NULL);
}


/* default constructor */
core_decode_t::core_decode_t(void)
{
}

/* default destructor */
core_decode_t::~core_decode_t()
{
}


/* load in all definitions */
#include "ZPIPE-decode.list"


class core_decode_t * decode_create(const char * decode_opt_string, struct core_t * core)
{
#define ZESTO_PARSE_ARGS
#include "ZPIPE-decode.list"

  fatal("unknown decode type \"%s\"",decode_opt_string);
#undef ZESTO_PARSE_ARGS
}
