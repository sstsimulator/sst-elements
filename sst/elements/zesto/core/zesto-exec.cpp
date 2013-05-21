/* zesto-exec.cpp - Zesto execution stage class
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
#include "zesto-memdep.h"
#include "zesto-bpred.h"
#include "zesto-fetch.h"
#include "zesto-decode.h"
#include "zesto-alloc.h"
#include "zesto-exec.h"
#include "zesto-commit.h"

void exec_reg_options(struct opt_odb_t * odb, struct core_knobs_t * knobs)
{
  /**********************/
  /* buffer/queue sizes */
  /**********************/
  opt_reg_int(odb, (char*)"-rs:size",(char*)"number of reservation station entries [DS]",
      &knobs->exec.RS_size, /*default*/ knobs->exec.RS_size, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-ldq:size",(char*)"number of load queue entries [DS]",
      &knobs->exec.LDQ_size, /*default*/ knobs->exec.LDQ_size, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-stq:size",(char*)"number of store queue entries [DS]",
      &knobs->exec.STQ_size, /*default*/ knobs->exec.STQ_size, /*print*/true,/*format*/NULL);

  opt_reg_int(odb, (char*)"-exec:width",(char*)"maximum issues from RS per cycle (equal to num exec ports) [DS]",
      &knobs->exec.num_exec_ports, /*default*/ knobs->exec.num_exec_ports, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-payload:depth",(char*)"number of cycles for payload RAM access (schedule-to-exec delay) [D]",
      &knobs->exec.payload_depth, /*default*/ knobs->exec.payload_depth, /*print*/true,/*format*/NULL);
  opt_reg_flag(odb, (char*)"-exec:tornado_breaker",(char*)"enable heuristic tornado breaker [D]",
      &knobs->exec.tornado_breaker, /*default*/ false, /*print*/true,/*format*/NULL);
  opt_reg_flag(odb, (char*)"-exec:partial_throttle",(char*)"enable load-issue throttling on partial matches [D]",
      &knobs->exec.throttle_partial, /*default*/ false, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-fp:penalty",(char*)"extra cycle(s) to forward results to FP cluster [D]",
      &knobs->exec.fp_penalty, /*default*/ knobs->exec.fp_penalty, /*print*/true,/*format*/NULL);

  /*****************************************************************/
  /* functional units (number of bindings implies number of units) */
  /*****************************************************************/
  opt_reg_int_list(odb, (char*)"-pb:ieu", (char*)"IEU port binding [DS]", knobs->exec.fu_bindings[FU_IEU],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_IEU].num_FUs, knobs->exec.fu_bindings[FU_IEU], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:jeu", (char*)"JEU port binding [DS]", knobs->exec.fu_bindings[FU_JEU],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_JEU].num_FUs, knobs->exec.fu_bindings[FU_JEU], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:imul", (char*)"IMUL port binding [DS]", knobs->exec.fu_bindings[FU_IMUL],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_IMUL].num_FUs, knobs->exec.fu_bindings[FU_IMUL], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:idiv", (char*)"IDIV port binding [DS]", knobs->exec.fu_bindings[FU_IDIV],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_IDIV].num_FUs, knobs->exec.fu_bindings[FU_IDIV], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:shift", (char*)"SHIFT port binding [DS]", knobs->exec.fu_bindings[FU_SHIFT],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_SHIFT].num_FUs, knobs->exec.fu_bindings[FU_SHIFT], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:fadd", (char*)"FADD port binding [DS]", knobs->exec.fu_bindings[FU_FADD],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_FADD].num_FUs, knobs->exec.fu_bindings[FU_FADD], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:fmul", (char*)"FMUL port binding [DS]", knobs->exec.fu_bindings[FU_FMUL],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_FMUL].num_FUs, knobs->exec.fu_bindings[FU_FMUL], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:fdiv", (char*)"FDIV port binding [DS]", knobs->exec.fu_bindings[FU_FDIV],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_FDIV].num_FUs, knobs->exec.fu_bindings[FU_FDIV], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:fcplx", (char*)"FCPLX port binding [DS]", knobs->exec.fu_bindings[FU_FCPLX],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_FCPLX].num_FUs, knobs->exec.fu_bindings[FU_FCPLX], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:lda", (char*)"LD port binding [DS]", knobs->exec.fu_bindings[FU_LD],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_LD].num_FUs, knobs->exec.fu_bindings[FU_LD], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:sta", (char*)"STA port binding [DS]", knobs->exec.fu_bindings[FU_STA],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_STA].num_FUs, knobs->exec.fu_bindings[FU_STA], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, (char*)"-pb:std", (char*)"STD port binding [DS]", knobs->exec.fu_bindings[FU_STD],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_STD].num_FUs, knobs->exec.fu_bindings[FU_STD], /* print */true, /* format */NULL, /* !accrue */false);

  /*****************************/
  /* functional unit latencies */
  /*****************************/

  opt_reg_int(odb, (char*)"-ieu:lat",(char*)"IEU execution latency [DS]",
      &knobs->exec.latency[FU_IEU], /*default*/ knobs->exec.latency[FU_IEU], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-jeu:lat",(char*)"JEU execution latency [DS]",
      &knobs->exec.latency[FU_JEU], /*default*/ knobs->exec.latency[FU_JEU], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-imul:lat",(char*)"IMUL execution latency [DS]",
      &knobs->exec.latency[FU_IMUL], /*default*/ knobs->exec.latency[FU_IMUL], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-idiv:lat",(char*)"IDIV execution latency [DS]",
      &knobs->exec.latency[FU_IDIV], /*default*/ knobs->exec.latency[FU_IDIV], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-shift:lat",(char*)"SHIFT execution latency [DS]",
      &knobs->exec.latency[FU_SHIFT], /*default*/ knobs->exec.latency[FU_SHIFT], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-fadd:lat",(char*)"FADD execution latency [DS]",
      &knobs->exec.latency[FU_FADD], /*default*/ knobs->exec.latency[FU_FADD], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-fmul:lat",(char*)"FMUL execution latency [DS]",
      &knobs->exec.latency[FU_FMUL], /*default*/ knobs->exec.latency[FU_FMUL], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-fdiv:lat",(char*)"FDIV execution latency [DS]",
      &knobs->exec.latency[FU_FDIV], /*default*/ knobs->exec.latency[FU_FDIV], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-fcplx:lat",(char*)"FCPLX execution latency [DS]",
      &knobs->exec.latency[FU_FCPLX], /*default*/ knobs->exec.latency[FU_FCPLX], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-lda:lat",(char*)"LD-agen execution latency [DS]",
      &knobs->exec.latency[FU_LD], /*default*/ knobs->exec.latency[FU_LD], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-sta:lat",(char*)"ST-agen execution latency [DS]",
      &knobs->exec.latency[FU_STA], /*default*/ knobs->exec.latency[FU_STA], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-std:lat",(char*)"ST-data execution latency [DS]",
      &knobs->exec.latency[FU_STD], /*default*/ knobs->exec.latency[FU_STD], /*print*/true,/*format*/NULL);

  /*******************************/
  /* functional unit issue rates */
  /*******************************/

  opt_reg_int(odb, (char*)"-ieu:rate",(char*)"IEU execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_IEU], /*default*/ knobs->exec.issue_rate[FU_IEU], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-jeu:rate",(char*)"JEU execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_JEU], /*default*/ knobs->exec.issue_rate[FU_JEU], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-imul:rate",(char*)"IMUL execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_IMUL], /*default*/ knobs->exec.issue_rate[FU_IMUL], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-idiv:rate",(char*)"IDIV execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_IDIV], /*default*/ knobs->exec.issue_rate[FU_IDIV], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-shift:rate",(char*)"SHIFT execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_SHIFT], /*default*/ knobs->exec.issue_rate[FU_SHIFT], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-fadd:rate",(char*)"FADD execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_FADD], /*default*/ knobs->exec.issue_rate[FU_FADD], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-fmul:rate",(char*)"FMUL execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_FMUL], /*default*/ knobs->exec.issue_rate[FU_FMUL], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-fdiv:rate",(char*)"FDIV execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_FDIV], /*default*/ knobs->exec.issue_rate[FU_FDIV], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-fcplx:rate",(char*)"FCPLX execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_FCPLX], /*default*/ knobs->exec.issue_rate[FU_FCPLX], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-lda:rate",(char*)"LD-agen execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_LD], /*default*/ knobs->exec.issue_rate[FU_LD], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-sta:rate",(char*)"ST-agen execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_STA], /*default*/ knobs->exec.issue_rate[FU_STA], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, (char*)"-std:rate",(char*)"ST-data execution issue rate [DS]",
      &knobs->exec.issue_rate[FU_STD], /*default*/ knobs->exec.issue_rate[FU_STD], /*print*/true,/*format*/NULL);

  opt_reg_flag(odb, (char*)"-warm:caches",(char*)"warm caches during functional fast-forwarding [DS]",
      &knobs->memory.warm_caches, /*default*/ false, /*print*/true,/*format*/NULL);

  /*******************************/
  /* memory dependence predictor */
  /*******************************/
  opt_reg_string(odb, (char*)"-memdep",(char*)"memory dependence predictor configuration string [D]",
      &knobs->exec.memdep_opt_str, /*default*/ "lwt:LWT:4096:131072", /*print*/true,/*format*/NULL);
}


/* default constructor */
core_exec_t::core_exec_t(void):
  last_completed(0), last_completed_count(0)
{
}

/* default destructor */
core_exec_t::~core_exec_t()
{
}

/* update deadlock watchdog timestamp */
void core_exec_t::update_last_completed(tick_t now)
{
  last_completed = now;
}


/* load in all definitions */
#include "ZPIPE-exec.list"

class core_exec_t * exec_create(const char * exec_opt_string, struct core_t * core)
{
#define ZESTO_PARSE_ARGS
#include "ZPIPE-exec.list"

  fatal("unknown exec type \"%s\"",exec_opt_string);
#undef ZESTO_PARSE_ARGS
}
