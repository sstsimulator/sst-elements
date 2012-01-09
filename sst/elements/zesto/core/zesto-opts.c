/* zesto-opts.c - Zesto command-line options/knobs
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

#ifndef ZESTO_OPTS_C
#define ZESTO_OPTS_C

#include "simplescalar/thread.h"
#include "simplescalar/loader.h"
#include "simplescalar/sim.h"

#include "zesto-opts.h"
#include "zesto-core.h"
#include "zesto-oracle.h"
#include "zesto-fetch.h"
#include "zesto-decode.h"
#include "zesto-alloc.h"
#include "zesto-exec.h"
#include "zesto-commit.h"

#ifdef ZTRACE
FILE * ztrace_fp = NULL;
char * ztrace_filename = NULL;
#endif

/* check simulator-specific option values */
/*
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  if(max_uops && max_uops < max_insts)
    warn("-max:uops is less than -max:inst, so -max:inst is useless");

  if((max_uops || max_insts) && max_cycles)
    warn("instruction/uop limit and cycle limit defined; will exit on whichever occurs first");

  if((num_threads < 1) || (num_threads > MAX_CORES))
    fatal("-cores must be between 1 and %d (inclusive)",MAX_CORES);

  simulated_processes_remaining = num_threads;
}
*/

#ifdef ZTRACE

void ztrace_Mop_ID(const struct Mop_t * Mop)
{
//  fprintf(ztrace_fp,"%lld|M:%lld|",sim_cycle,Mop->oracle.seq);
  if(Mop->oracle.spec_mode)
    fprintf(ztrace_fp,"X|");
  else
    fprintf(ztrace_fp,".|");
}

void ztrace_uop_ID(const struct uop_t * uop)
{
//  fprintf(ztrace_fp,"%lld|u:%lld:%lld|", sim_cycle,
//          uop->decode.Mop_seq, (uop->decode.Mop_seq << UOP_SEQ_SHIFT) + uop->flow_index);
  if(uop->Mop && uop->Mop->oracle.spec_mode)
    fprintf(ztrace_fp,"X|");
  else
    fprintf(ztrace_fp,".|");
}


/* called by oracle when Mop first executes */
void ztrace_print(const struct Mop_t * Mop)
{
  if(ztrace_fp)
  {
    ztrace_Mop_ID(Mop);
    // core id, PC{virtual,physical}
    fprintf(ztrace_fp,"DEF|core=%d:virtPC=%x:physPC=%llx:op=",Mop->core->id,Mop->fetch.PC,v2p_translate(Mop->core->id,Mop->fetch.PC));
    // rep prefix and iteration
    if(Mop->fetch.inst.rep)
      fprintf(ztrace_fp,"rep{%d}",Mop->decode.rep_seq);
    // opcode name
    fprintf(ztrace_fp,"%s:",md_op2name[Mop->decode.op]);
    fprintf(ztrace_fp,"trap=%d:",Mop->decode.is_trap);

    // ucode flow length
    fprintf(ztrace_fp,"flow-length=%d\n",Mop->decode.flow_length);

    int i;
    int count=0;
    for(i=0;i<Mop->decode.flow_length;)
    {
      struct uop_t * uop = &Mop->uop[i];
      ztrace_uop_ID(uop);
      fprintf(ztrace_fp,"DEF");
      if(uop->decode.BOM && !uop->decode.EOM) fprintf(ztrace_fp,"-BOM");
      if(uop->decode.EOM && !uop->decode.BOM) fprintf(ztrace_fp,"-EOM");
      // core id, uop number within flow
      fprintf(ztrace_fp,"|core=%d:uop-number=%d:",Mop->core->id,count);
      // opcode name
      fprintf(ztrace_fp,"op=%s",md_op2name[uop->decode.op]);
      if(uop->decode.in_fusion)
      {
        fprintf(ztrace_fp,"-f");
        if(uop->decode.is_fusion_head)
          fprintf(ztrace_fp,"H"); // fusion head
        else
          fprintf(ztrace_fp,"b"); // fusion body
      }

      // register identifiers
      fprintf(ztrace_fp,":odep=%d:i0=%d:i1=%d:i2=%d:",
          uop->decode.odep_name,
          uop->decode.idep_name[0],
          uop->decode.idep_name[1],
          uop->decode.idep_name[2]);

      // load/store address and size
      if(uop->decode.is_load || uop->decode.is_sta)
        fprintf(ztrace_fp,"VA=%lx:PA=%llx:mem-size=%d:fault=%d",(long unsigned int)uop->oracle.virt_addr,uop->oracle.phys_addr,uop->decode.mem_size,uop->oracle.fault);

      fprintf(ztrace_fp,"\n");

      i += Mop->uop[i].decode.has_imm?3:1;
      count++;
    }
  }
}

void ztrace_print(const struct Mop_t * Mop, const char * fmt, ... )
{
  if(ztrace_fp)
  {
    va_list v;
    va_start(v, fmt);

    ztrace_Mop_ID(Mop);
    myvfprintf(ztrace_fp, fmt, v);
    fprintf(ztrace_fp,"\n");
  }
}

void ztrace_print(const struct uop_t * uop, const char * fmt, ... )
{
  if(ztrace_fp)
  {
    va_list v;
    va_start(v, fmt);

    ztrace_uop_ID(uop);
    myvfprintf(ztrace_fp, fmt, v);
    fprintf(ztrace_fp,"\n");
  }
}

void ztrace_print(const char * fmt, ... )
{
  if(ztrace_fp)
  {
    va_list v;
    va_start(v, fmt);

//    fprintf(ztrace_fp,"%lld|",sim_cycle);
    myvfprintf(ztrace_fp, fmt, v);
    fprintf(ztrace_fp,"\n");
  }
}

void ztrace_print_start(const struct uop_t * uop, const char * fmt, ... )
{
  if(ztrace_fp)
  {
    va_list v;
    va_start(v, fmt);

    ztrace_uop_ID(uop);
    myvfprintf(ztrace_fp, fmt, v);
  }
}

void ztrace_print_cont(const char * fmt, ... )
{
  if(ztrace_fp)
  {
    va_list v;
    va_start(v, fmt);

    myvfprintf(ztrace_fp, fmt, v);
  }
}

void ztrace_print_finish(const char * fmt, ... )
{
  if(ztrace_fp)
  {
    va_list v;
    va_start(v, fmt);

    myvfprintf(ztrace_fp, fmt, v);
    fprintf(ztrace_fp,"\n");
  }
}

#endif

#endif
