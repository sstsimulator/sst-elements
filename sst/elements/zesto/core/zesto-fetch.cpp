/* zesto-fetch.cpp - Zesto fetch stage class
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

void fetch_reg_options(struct opt_odb_t * odb, struct core_knobs_t * knobs)
{
  /* branch prediction */
  opt_reg_string_list(odb, (char*)"-bpred", (char*)"bpred configuration string(s) [DS]",
      knobs->fetch.bpred_opt_str, MAX_HYBRID_BPRED, &knobs->fetch.num_bpred_components, knobs->fetch.bpred_opt_str, /* print */true, /* format */NULL, /* !accrue */false);

  opt_reg_string(odb, (char*)"-bpred:fusion",(char*)"fusion/meta-prediction algorithm configurations string [DS]",
      &knobs->fetch.fusion_opt_str, /*default*/ (char*)"none", /*print*/true,/*format*/NULL);

  opt_reg_string(odb, (char*)"-bpred:btb",(char*)"branch target buffer configuration configuration string [DS]",
      &knobs->fetch.dirjmpbtb_opt_str, /*default*/ (char*)"btac:BTB:512:4:8:l", /*print*/true,/*format*/NULL);

  opt_reg_string(odb, (char*)"-bpred:ibtb",(char*)"indirect branch target buffer configuration string [DS]",
      &knobs->fetch.indirjmpbtb_opt_str, /*default*/ (char*)"2levbtac:iBTB:1:8:1:128:4:8:l", /*print*/true,/*format*/NULL);

  opt_reg_string(odb, (char*)"-bpred:ras",(char*)"return address stack predictor configuration string [DS]",
      &knobs->fetch.ras_opt_str, /*default*/ (char*)"stack:RAS:16", /*print*/true,/*format*/NULL);

  opt_reg_int(odb, (char*)"-jeclear:delay",(char*)"additional latency from branch-exec to jeclear [D]",
      &knobs->fetch.jeclear_delay, /*default*/ 1, /*print*/true,/*format*/NULL);

  opt_reg_int(odb, (char*)"-byteQ:size",(char*)"number of entries in byteQ [DS]",
      &knobs->fetch.byteQ_size, /*default*/ knobs->fetch.byteQ_size, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-byteQ:linesize",(char*)"linesize of byteQ (bytes) [DS]",
      &knobs->fetch.byteQ_linesize, /*default*/ knobs->fetch.byteQ_linesize, /*print*/true,/*format*/NULL);

  opt_reg_int(odb, (char*)"-predecode:depth",(char*)"number of stages in predecode pipe [D]",
      &knobs->fetch.depth, /*default*/ knobs->fetch.depth, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-predecode:width",(char*)"width of predecode pipe (Macro-ops) [D]",
      &knobs->fetch.width, /*default*/ knobs->fetch.width, /*print*/true,/*format*/NULL);

  opt_reg_int(odb, (char*)"-IQ:size",(char*)"size of instruction queue (Macro-ops - placed between predecode and decode) [D]",
      &knobs->fetch.IQ_size, /*default*/ knobs->fetch.IQ_size, /*print*/true,/*format*/NULL);

  opt_reg_flag(odb, (char*)"-warm:bpred",(char*)"warm branch predictors during functional fast-forwarding [DS]",
      &knobs->fetch.warm_bpred, /*default*/ false, /*print*/true,/*format*/NULL);
}


/* default constructor */
core_fetch_t::core_fetch_t(void): bogus(false)
{
}

/* default destructor */
core_fetch_t::~core_fetch_t()
{
}

/* reset bpred stats */
void core_fetch_t::reset_bpred_stats(void)
{
  if(bpred)
    bpred->reset_stats();
}


/* load in all definitions */
#include "ZPIPE-fetch.list"


class core_fetch_t * fetch_create(const char * fetch_opt_string, struct core_t * core)
{
#define ZESTO_PARSE_ARGS
#include "ZPIPE-fetch.list"

  fatal("unknown fetch engine type \"%s\"",fetch_opt_string);
#undef ZESTO_PARSE_ARGS
}
