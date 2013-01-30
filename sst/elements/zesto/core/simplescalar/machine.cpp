/* x86.c (machine.c) - x86 ISA definition routines
 *
 * SimpleScalar Ô Tool Suite
 * © 1994-2003 Todd M. Austin, Ph.D. and SimpleScalar, LLC
 * All Rights Reserved.
 * 
 * THIS IS A LEGAL DOCUMENT BY DOWNLOADING SIMPLESCALAR, YOU ARE AGREEING TO
 * THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted as
 * described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express or
 * implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged.  SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship purposes
 * provided that this notice in its entirety accompanies all copies. Copies of
 * the modified software can be delivered to persons who use it solely for
 * nonprofit, educational, noncommercial research, and noncommercial
 * scholarship purposes provided that this notice in its entirety accompanies
 * all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a copy
 * of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright © 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */

#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "eval.h"
#include "regs.h"
#include "memory.h"
#include "sim.h"
#include "x86flow.def"
#include "core/zesto-structs.h"
//#include "qsim.h"

#ifndef SYMCAT
#define SYMCAT(XX,YY)	XX ## YY
#endif

#ifndef SYMCAT3
#define SYMCAT3(XX,YY,ZZ)	XX ## YY ## ZZ
#endif

#if !defined(HOST_HAS_QWORD)
#error SimpleScalar/x86 only builds on hosts with builtin qword support...
#error Try building with GNU GCC, as it supports qwords on most machines.
#endif

#define RO        (Mop->fetch.inst.code[Mop->fetch.inst.npfx + Mop->fetch.inst.nopc - 1] & 0x07)
#define R        ((Mop->fetch.inst.code[Mop->fetch.inst.npfx + Mop->fetch.inst.nopc] >> 3) & 0x07)
#define RM        (Mop->fetch.inst.code[Mop->fetch.inst.npfx + Mop->fetch.inst.nopc] & 0x07)


unsigned long long timestamp[MAX_CORES] = 
 {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; // for RDTSC

/* preferred nop instruction definition */
const md_inst_t MD_NOP_INST = { {0x90} };      	/* NOP instruction */
/* set parameters of MD_NOP_INST */


/* opcode mask -> enum md_opcodem, used by decoder */
enum md_opcode md_mask2op[MD_MAX_MASK+1];
unsigned int md_opoffset[OP_MAX];

enum mem_type {NOT_MEM=-1,MEM_READ=0, MEM_WRITE=1};

/* enum md_opcode -> mask for decoding next level */
const unsigned int md_opmask[OP_MAX] = {
  0, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) 0,
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) 0,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK) MASK,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> shift for decoding next level */
const unsigned int md_opshift[OP_MAX] = {
  0, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) 0,
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) 0,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK) SHIFT,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> description string */
const char *md_op2name[OP_MAX] = {
  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) NAME,
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) NAME,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NAME,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> enum name string */
const char *md_op2enum[OP_MAX] = {
  "OP_NA", /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) #OP,
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) #OP,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) #OP,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> opcode operand format, used by disassembler */
const char *md_op2format[OP_MAX] = {
  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) OPFORM,
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) OPFORM,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NULL,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> enum md_fu_class, used by performance simulators */
const enum md_fu_class md_op2fu[OP_MAX] = {
  FU_NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) RES,
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) RES,
#define DEFLINK(OP,MSK,NAME, MASK,SHIFT) FU_INVALID,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_fu_class -> description string */
/* ----- SoM ----- */
/* Note: FU names does not match the enum names used in Zesto */
const char *md_fu2name[NUM_FU_CLASSES] = {
  NULL, /* NA */
  "FU_IEU",
  "FU_JEU",
  "FU_IMUL",
  "FU_SHIFT",
  "FU_FADD",
  "FU_FMUL",
  "FU_FCPLX",
  "FU_IDIV",
  "FU_FDIV",
  "FU_LD",
  "FU_STA",
  "FU_STD",
/*
  "int-exec-unit",
  "jump-exec-unit",
  "int-multiply",
  "int-shift",
  "FP-add",
  "FP-multiply",
  "FP-complex",
  "FP-divide",
  "load-port",
  "sta-port",
  "std-port",
*/
};
/* ----- EoM ----- */

#define _OP		MODE_OPER32
#define _AD		MODE_ADDR32
#define PFX_MODE(X)	((X) & 0x07)

#define _CS		(SEG_CS << 3)
#define _SS		(SEG_SS << 3)
#define _DS		(SEG_DS << 3)
#define _ES		(SEG_ES << 3)
#define _FS		(SEG_FS << 3)
#define _GS		(SEG_GS << 3)
#define PFX_SEG(X)	(((X) >> 3) & 0x07)

#define _LK		(1 << 6)
#define _WT		_LK
#define PFX_LOCK(X)	(((X) >> 6) & 1)

#define _NZ		(REP_REPNZ << 7)
#define _Z		(REP_REPZ << 7)
#define PFX_REP(X)	(((X) >> 7) & 0x03)

const static word_t pfxtab[256] = {
  /* 0x00 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x08 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x10 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x18 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x20 */  0,   0,   0,   0,   0,   0, _ES,   0,
  /* 0x28 */  0,   0,   0,   0,   0,   0, _CS,   0,
  /* 0x30 */  0,   0,   0,   0,   0,   0, _SS,   0,
  /* 0x38 */  0,   0,   0,   0,   0,   0, _DS,   0,
  /* 0x40 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x48 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x50 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x58 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x60 */  0,   0,   0,   0, _FS, _GS, _OP, _AD,
  /* 0x68 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x70 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x78 */  0,   0,   0,   0,   0,   0,   0,   0,

  /* 0x80 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x88 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x90 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0x98 */  0,   0,   0,   _WT, 0,   0,   0,   0,
  /* 0xa0 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0xa8 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0xb0 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0xb8 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0xc0 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0xc8 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0xd0 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0xd8 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0xe0 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0xe8 */  0,   0,   0,   0,   0,   0,   0,   0,
  /* 0xf0 */_LK,   0, _NZ,  _Z,   0,   0,   0,   0,
  /* 0xf8 */  0,   0,   0,   0,   0,   0,   0,   0
};


  int
md_cc_eval(const int cond, const dword_t aflags, bool * bogus)
{
  int res;

  switch (cond)
  {
    case CC_O:		res = !!(aflags & OF); break;
    case CC_NO:		res = !(aflags & OF); break;
    case CC_B:		res = !!(aflags & CF); break;
    case CC_NB:		res = !(aflags & CF); break;
    case CC_E:		res = !!(aflags & ZF); break;
    case CC_NE:		res = !(aflags & ZF); break;
    case CC_BE:		res = (aflags & CF) || (aflags & ZF); break;
    case CC_NBE:	res = !(aflags & CF) && !(aflags & ZF); break;
    case CC_S:		res = !!(aflags & SF); break;
    case CC_NS:		res = !(aflags & SF); break;
    case CC_P:		warnonce("PF flags probed"); res = !!(aflags & PF); break; // skumar - warn changed to warnonce
    case CC_NP:		warnonce("PF flags probed"); res = !(aflags & PF); break; 
    case CC_L:		res = !(aflags & SF) != !(aflags & OF); break;
    case CC_NL:		res = !(aflags & SF) == !(aflags & OF); break;
    case CC_LE:
                  res = (aflags & ZF) || (!(aflags & SF) != !(aflags & OF)); break;
    case CC_NLE:
                  res = !(aflags & ZF) && (!(aflags & SF) == !(aflags & OF)); break;
    default: if(bogus)
             {
               *bogus = TRUE;
               res = 0;
             }
             else
               panic("bogus CC condition: %d", cond);
  }
  return res;
}

  int
md_fcc_eval(int cond, dword_t aflags, bool * bogus)
{
  int res;

  switch (cond)
  {
    case FCC_B:		res = !!(aflags & CF); break;
    case FCC_E:		res = !!(aflags & ZF); break;
    case FCC_BE:	res = ((aflags & CF) || (aflags & ZF)); break;
    case FCC_U:		res = !!(aflags & PF); break;
    case FCC_NB:	res = !(aflags & CF); break;
    case FCC_NE:	res = !(aflags & ZF); break;
    case FCC_NBE:	res = (!(aflags & CF) && !(aflags & ZF)); break;
    case FCC_NU:	res = !(aflags & PF); break;
    default: if(bogus)
             {
               *bogus = TRUE;
               res = 0;
             }
             else
               panic("bogus FCC condition: %d", cond);
  }
  return res;
}

/* decode an instruction */
  enum md_opcode
md_decode(const byte_t mode, struct md_inst_t *inst, const int set_nop)
{
  byte_t *pinst = inst->code, modbyte;
  int pfx, npfx, nopc, modrm, flags, sib;
  int base, index, scale, ndisp, nimm;
  sdword_t disp, imm;
  enum md_opcode op;
  unsigned char real_op = 0;

  /* decode current execution mode */
  pfx = 0; npfx = 0;
  while (pfxtab[*pinst])
  {
    /* NOTE: multiple identical prefi (sp?) do not negate the previous */
    pfx |= pfxtab[*pinst++];
    npfx++;
  }
  inst->npfx = npfx;
  inst->mode = PFX_MODE(pfx) ^ mode; /* FIXME: swizzle in mode at runtime... */
  inst->rep = PFX_REP(pfx);
  inst->lock = PFX_LOCK(pfx);
  inst->seg = PFX_SEG(pfx);

  /* decode the instruction opcode */
  if (*pinst == 0x0f)
  {
    //fprintf(stdout, " Two byte opcode ");
    /* decode escape, advance to opcode */
    op = md_mask2op[*pinst++];
    /* decode opcode, advance to (possible) modrm */
    real_op = *pinst;
    op = md_mask2op[(((*pinst++ >> md_opshift[op]) & md_opmask[op])
        + md_opoffset[op])];
    nopc = 2;
  }
  else
  {
    /* decode opcode, advance to (possible) modrm */
    real_op = *pinst;
    op = md_mask2op[*pinst++];
    nopc = 1;
  }
  inst->nopc = nopc;

  /* decode possible modrm */
  modrm = 0;
  while (md_opmask[op])
  {
    real_op = *pinst;
    op = md_mask2op[(((*pinst >> md_opshift[op]) & md_opmask[op])
        + md_opoffset[op])];
    modrm = 1;
  }



  ///////fprintf(stdout,"\n op= %x nopc= %d modrm= %d ",op,nopc,modrm);
  if( (inst->code[0]==0x9b) && (inst->qemu_len==1) )
  {
  	//fprintf(stderr,"\n Floating point wait signal received  so passing a no-op: ");
  	//for (int i = 0; i < inst->len; i++) 
  		//fprintf(stderr,"%x ", inst->code[i]);
  	return MD_NOP_OP;
  }



  if (op == NA)
  {
    //if ( set_nop == 0 )
      //fprintf(stdout,"\nInvalid opcode so returning no-op 0x %d",inst->len);
      //for (int i = 0; i < inst->qemu_len; i++) 
	//fprintf(stdout,"%x ", inst->code[i]);
      //fatal("invalid opcode: 0x%x:%x", real_op,op);
    //else 
    //{
      return MD_NOP_OP;
    //}
    inst->modrm = 0;
    inst->sib = 0;
    inst->base = -1;
    inst->index = -1;
    inst->ndisp = 0;
    inst->disp = 0;
    inst->nimm = 0;
    inst->imm = 0;
    inst->len = npfx + nopc + modrm;
    return op;
  }

  flags = MD_OP_FLAGS(op);

  /* adjust repeat */
  if (inst->rep == REP_REPZ)
  {
    if (flags & F_REP)
      inst->rep = REP_REP;
  }

  inst->modrm = !(flags & F_NOMOD);
  assert (modrm == !(flags & F_NOMOD));

  /* decode addressing mode */
  sib = FALSE;
  base = index = -1;
  scale = 0;
  ndisp = 0; disp = 0;
  modbyte = *pinst;

  if (modrm && MODRM_MOD(modbyte) != 3)
  {
    if (inst->mode & MODE_ADDR32)
    {
      /* 32-bit addressing mode */
      switch (MODRM_MOD(modbyte))
      {
        case 0x00:
          if (MODRM_RM(modbyte) == 4)
          {
            /* sib byte included... */
            sib = TRUE;
          }
          else if (MODRM_RM(modbyte) == 5)
          {
            /* 32-bit direct addressing */
            ndisp = 4;
          }
          else
          {
            /* register indirect addressing */
            base = MODRM_RM(modbyte);
          }
          break;

        case 0x01:
          if (MODRM_RM(modbyte) == 4)
          {
            /* sib byte included... */
            sib = TRUE;
            ndisp = 1;
          }
          else
          {
            /* 8-bit displaced addressing */
            base = MODRM_RM(modbyte);
            ndisp = 1;
          }
          break;

        case 0x02:
          if (MODRM_RM(modbyte) == 4)
          {
            /* sib byte included... */
            sib = TRUE;
            ndisp = 4;
          }
          else
          {
            /* 32-bit displaced addressing */
            base = MODRM_RM(modbyte);
            ndisp = 4;
          }
          break;

        case 0x03:
          panic("not an addressing mode");
      }

      if (sib)
      {
        byte_t sib = *++pinst;

        /* continue sib byte decode */
        if (SIB_BASE(sib) == 5 && MODRM_MOD(modbyte) == 0)
        {
          if (SIB_INDEX(sib) != 4)
            index = SIB_INDEX(sib);
          scale = SIB_SCALE(sib);
          ndisp = 4;
        }
        else
        {
          base = SIB_BASE(sib);
          if (SIB_INDEX(sib) != 4)
            index = SIB_INDEX(sib);
          scale = SIB_SCALE(sib);
        }
      }
    }
    else
    {
      /* 16-bit addressing mode */
      switch (MODRM_MOD(modbyte))
      {
        case 0x00:
          if (MODRM_RM(modbyte) == 6)
          {
            /* 16-bit direct addressing */
            ndisp = 2;
            goto done;
          }
          break;

        case 0x01:
          /* 8-bit displacement */
          ndisp = 1;
          break;

        case 0x02:
          /* 16-bit displacement */
          ndisp = 2;
          break;

        case 0x03:
          panic("not an addressing mode");
      }

      /* determine base and index registers */
      switch (MODRM_RM(modbyte))
      {
        case 0x00:	base = MD_REG_BX; index = MD_REG_SI; break;
        case 0x01:	base = MD_REG_BX; index = MD_REG_DI; break;
        case 0x02:	base = MD_REG_BP; index = MD_REG_SI; break;
        case 0x03:	base = MD_REG_BP; index = MD_REG_DI; break;
        case 0x04:	index = MD_REG_SI; break;
        case 0x05:	index = MD_REG_DI; break;
        case 0x06:	base = MD_REG_BP; break;
        case 0x07:	base = MD_REG_BX; break;
      }
done:
      ;
    }

    /* extract (possible) displacement, all disps are sign-extended */
    {
      int dispidx = npfx + nopc + modrm + sib;

      if (inst->mode & MODE_ADDR32)
      {
        switch (ndisp)
        {
          case 0:
            disp = 0;
            break;
          case 1:
            disp = (sdword_t)(*(sbyte_t *)(inst->code + dispidx));
            break;
          case 4:
            disp = *(sdword_t *)(inst->code + dispidx);
            break;
          case 2: default:
            panic("boom: invalid addressing mode");
        }
      }
      else /* 16-bit addressing mode */
      {
        switch (ndisp)
        {
          case 0:
            disp = 0;
            break;
          case 1:
            disp = (dword_t)(sword_t)(*(sbyte_t *)(inst->code + dispidx));
            break;
          case 2:
            disp = (dword_t)(*(sword_t *)(inst->code + dispidx));
            break;
          case 4: default:
            panic("boom: invalid addressing mode");
        }
      }
    }
  }
  inst->sib = sib;
  inst->base = base;
  inst->index = index;
  inst->scale = scale;
  inst->ndisp = ndisp;
  inst->disp = disp;

  /* decode (possible) immediate value */
  nimm = imm = 0;
  if (flags & F_HASIMM)
  {
    int immidx = npfx + nopc + modrm + sib + ndisp;

    nimm = F_IMMSZ(inst->mode, flags);
    if (flags & F_UIMM)
    {
      /* unsigned immediate */
      switch (nimm)
      {
        case 0: imm = 0; break;
        case 1: imm = (dword_t)(*(byte_t *)(inst->code + immidx)); break;
        case 2: imm = (dword_t)(*(word_t *)(inst->code + immidx)); break;
        case 4: imm = *(dword_t *)(inst->code + immidx); break;
        default: panic("boom");
      }
    }
    else
    {
      /* signed immediate */
      switch (nimm)
      {
        case 0: imm = 0; break;
        case 1: imm = (sdword_t)(*(sbyte_t *)(inst->code + immidx)); break;
        case 2: imm = (sdword_t)(*(sword_t *)(inst->code + immidx)); break;
        case 4: imm = *(sdword_t *)(inst->code + immidx); break;
        default: panic("boom");
      }
    }
  }
  inst->nimm = nimm;
  inst->imm = imm;

  /* compute total instruction length */
  inst->len = npfx + nopc + modrm + sib + ndisp + nimm;

  /* done, return the enumerator for this instruction */
  return op;
}

/* lots 'o string defs */
const char *md_gprb_names[16] =
{ "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH",
  "TMP0.b", "TMP1.b", "TMP2.b", "???", "???", "???", "???", "ZERO.b" };
const char *md_gprw_names[16] =
{ "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI",
  "TMP0.w", "TMP1.w", "TMP2.w", "???", "???", "???", "???", "ZERO.w" };
const char *md_gprd_names[16] =
{ "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI",
  "TMP0.d", "TMP1.d", "TMP2.d", "???", "???", "???", "???", "ZERO.d" };

const char *md_fpre_names[16] =
{ "ST0", "ST1", "ST2", "ST3", "ST4", "ST5", "ST6", "ST7",
  "FTMP0.e", "FTMP1.e", "FTMP2.e", "???", "???", "???", "???", "???" };

const char *md_fpstk_names[16] =
{ "", ",stk++", ",stk++,stk++", ",stk--", ",???", ",???", ",???", ",???",
  ",???", ",???", ",???", ",???", ",???", ",???", ",???", ",???" };

const char *md_cc_names[16] =
{ "o", "no", "b", "nb", "e", "ne", "be", "nbe",
  "s", "ns", "p", "np", "l", "nl", "le", "nle" };

const char *md_lc_names[8] =
{ "1", "l2t", "l2e", "pi", "lg2", "ln2", "z", "???" };

/* disassemble an addressing mode */
  void
md_print_addr(md_inst_t inst,		/* instruction to disassemble */
    md_addr_t pc,             /* addr of inst, used for PC-rels */
    FILE *stream)
{
  const char **regs = NULL;

  if (inst.base < 0 && inst.index < 0 && inst.scale == 0 && inst.disp != 0)
    fprintf(stream, "[0x%08x]", inst.disp);
  else
  {
    if (inst.disp)
      fprintf(stream, "0x%x", inst.disp);
    fprintf(stream, "[");

    if (inst.mode & MODE_ADDR32)
      regs = md_gprd_names;
    else
      regs = md_gprw_names;

    if (inst.base >= 0)
    {
      if (inst.index >= 0)
      {
        if (inst.scale != 0)
          fprintf(stream, "%s+%s*%d",
              regs[inst.base], regs[inst.index],
              1 << inst.scale);
        else
          fprintf(stream, "%s+%s", regs[inst.base], regs[inst.index]);
      }
      else
        fprintf(stream, "%s", regs[inst.base]);
    }
    else if (inst.index >= 0)
    {
      if (inst.scale != 0)
        fprintf(stream, "%s*%d", regs[inst.index], 1 << inst.scale);
      else
        fprintf(stream, "%s", regs[inst.index]);
    }
    fprintf(stream, "]");
  }
}


/* generate an instruction format string */
  void
md_print_ifmt(const char *fmt,
    const struct Mop_t * Mop,		/* instruction to disassemble */
    FILE *stream)
{
  const char *s = fmt;
  md_addr_t pc = Mop->fetch.PC;

  while (*s)
  {
    if (*s == '%')
    {
      s++;

      if (!strncmp(s, "eA", 2))
      {
        if (Mop->fetch.inst.mode & MODE_OPER32)
          fputs("EAX", stream);
        else
          fputs("AX", stream);
      }
      else if (!strncmp(s, "eC", 2))
      {
        if (Mop->fetch.inst.mode & MODE_ADDR32) /* FIXME: kludge... */
          fputs("ECX", stream);
        else
          fputs("CX", stream);
      }

      else if (!strncmp(s, "Qb", 2))
        fputs(md_gprb_names[RO], stream);
      else if (!strncmp(s, "Qw", 2))
        fputs(md_gprw_names[RO], stream);
      else if (!strncmp(s, "Qd", 2))
        fputs(md_gprd_names[RO], stream);
      else if (!strncmp(s, "Qv", 2))
      {
        if (Mop->fetch.inst.mode & MODE_OPER32)
          fputs(md_gprd_names[RO], stream);
        else
          fputs(md_gprw_names[RO], stream);
      }

      else if (!strncmp(s, "Rb", 2))
        fputs(md_gprb_names[R], stream);
      else if (!strncmp(s, "Rw", 2))
        fputs(md_gprw_names[R], stream);
      else if (!strncmp(s, "Rd", 2))
        fputs(md_gprd_names[R], stream);
      else if (!strncmp(s, "Rv", 2))
      {
        if (Mop->fetch.inst.mode & MODE_OPER32)
          fputs(md_gprd_names[R], stream);
        else
          fputs(md_gprw_names[R], stream);
      }

      else if (!strncmp(s, "Nb", 2))
        fputs(md_gprb_names[RM], stream);
      else if (!strncmp(s, "Nw", 2))
        fputs(md_gprw_names[RM], stream);
      else if (!strncmp(s, "Nd", 2))
        fputs(md_gprd_names[RM], stream);
      else if (!strncmp(s, "Nv", 2))
      {
        if (Mop->fetch.inst.mode & MODE_OPER32)
          fputs(md_gprd_names[RM], stream);
        else
          fputs(md_gprw_names[RM], stream);
      }

      else if (!strncmp(s, "Si", 2))
        fprintf(stream, "ST(%d)", STI);

      else if (!strncmp(s, "Mb", 2))
      {
        fputs("BYTE PTR ", stream);
        md_print_addr(Mop->fetch.inst, pc, stream);
      }
      else if (!strncmp(s, "Mw", 2))
      {
        fputs("WORD PTR ", stream);
        md_print_addr(Mop->fetch.inst, pc, stream);
      }
      else if (!strncmp(s, "Md", 2))
      {
        fputs("DWORD PTR ", stream);
        md_print_addr(Mop->fetch.inst, pc, stream);
      }
      else if (!strncmp(s, "Mq", 2))
      {
        fputs("QWORD PTR ", stream);
        md_print_addr(Mop->fetch.inst, pc, stream);
      }
      else if (!strncmp(s, "Ms", 2))
      {
        fputs("REAL4 PTR ", stream);
        md_print_addr(Mop->fetch.inst, pc, stream);
      }
      else if (!strncmp(s, "Mt", 2))
      {
        fputs("REAL8 PTR ", stream);
        md_print_addr(Mop->fetch.inst, pc, stream);
      }
      else if (!strncmp(s, "Me", 2))
      {
        fputs("REAL12 PTR ", stream);
        md_print_addr(Mop->fetch.inst, pc, stream);
      }
      else if (!strncmp(s, "Mx", 2))
        md_print_addr(Mop->fetch.inst, pc, stream);
      else if (!strncmp(s, "Mv", 2))
      {
        if (Mop->fetch.inst.mode & MODE_OPER32)
        {
          fputs("DWORD PTR ", stream);
          md_print_addr(Mop->fetch.inst, pc, stream);
        }
        else
        {
          fputs("WORD PTR ", stream);
          md_print_addr(Mop->fetch.inst, pc, stream);
        }
      }

      else if (!strncmp(s, "Xb", 2))
      {
        fprintf(stream, "BYTE PTR ");
        if (Mop->fetch.inst.mode & MODE_ADDR32)
          fprintf(stream, "DS:[ESI]");
        else
          fprintf(stream, "DS:[SI]");
      }
      else if (!strncmp(s, "Xv", 2))
      {
        if (Mop->fetch.inst.mode & MODE_OPER32)
          fprintf(stream, "DWORD PTR ");
        else
          fprintf(stream, "WORD PTR ");
        if (Mop->fetch.inst.mode & MODE_ADDR32)
          fprintf(stream, "DS:[ESI]");
        else
          fprintf(stream, "DS:[SI]");
      }
      else if (!strncmp(s, "Yb", 2))
      {
        fprintf(stream, "BYTE PTR ");
        if (Mop->fetch.inst.mode & MODE_ADDR32)
          fprintf(stream, "ES:[EDI]");
        else
          fprintf(stream, "ES:[DI]");
      }
      else if (!strncmp(s, "Yv", 2))
      {
        if (Mop->fetch.inst.mode & MODE_OPER32)
          fprintf(stream, "DWORD PTR ");
        else
          fprintf(stream, "WORD PTR ");
        if (Mop->fetch.inst.mode & MODE_ADDR32)
          fprintf(stream, "ES:[EDI]");
        else
          fprintf(stream, "ES:[DI]");
      }

      else if (!strncmp(s, "Ib", 2))
        fprintf(stream, "0x%02x", (dword_t)(byte_t)Mop->fetch.inst.imm);
      else if (!strncmp(s, "Iw", 2))
        fprintf(stream, "0x%04x", (dword_t)(word_t)Mop->fetch.inst.imm);
      else if (!strncmp(s, "Id", 2))
        fprintf(stream, "0x%08x", (sdword_t)Mop->fetch.inst.imm);
      else if (!strncmp(s, "Iv", 2))
      {
        if (Mop->fetch.inst.mode & MODE_OPER32)
          fprintf(stream, "0x%08x", (sdword_t)Mop->fetch.inst.imm);
        else
          fprintf(stream, "0x%04x", (dword_t)(word_t)Mop->fetch.inst.imm);
      }
      else if (!strncmp(s, "Ia", 2))
      {
        if (Mop->fetch.inst.mode & MODE_OPER32)
          fprintf(stream, "0x%08x", (sdword_t)Mop->fetch.inst.imm);
        else
          fprintf(stream, "0x%04x", (dword_t)(word_t)Mop->fetch.inst.imm);
      }

      else if (!strncmp(s, "Ob", 2))
      {
        if (Mop->fetch.inst.mode & MODE_ADDR32)
          fprintf(stream, "BYTE PTR [0x%08x]", Mop->fetch.inst.imm);
        else
          fprintf(stream, "BYTE PTR [0x%04x]", Mop->fetch.inst.imm);
      }
      else if (!strncmp(s, "Ow", 2))
      {
        if (Mop->fetch.inst.mode & MODE_ADDR32)
          fprintf(stream, "WORD PTR [0x%08x]", Mop->fetch.inst.imm);
        else
          fprintf(stream, "WORD PTR [0x%04x]", Mop->fetch.inst.imm);
      }
      else if (!strncmp(s, "Od", 2))
      {
        if (Mop->fetch.inst.mode & MODE_ADDR32)
          fprintf(stream, "DWORD PTR [0x%08x]", Mop->fetch.inst.imm);
        else
          fprintf(stream, "DWORD PTR [0x%04x]", Mop->fetch.inst.imm);
      }
      else if (!strncmp(s, "Ov", 2))
      {
        if (Mop->fetch.inst.mode & MODE_OPER32)
        {
          if (Mop->fetch.inst.mode & MODE_ADDR32)
            fprintf(stream, "DWORD PTR [0x%08x]", Mop->fetch.inst.imm);
          else
            fprintf(stream, "DWORD PTR [0x%04x]", Mop->fetch.inst.imm);
        }
        else
        {
          if (Mop->fetch.inst.mode & MODE_ADDR32)
            fprintf(stream, "WORD PTR [0x%08x]", Mop->fetch.inst.imm);
          else
            fprintf(stream, "WORD PTR [0x%04x]", Mop->fetch.inst.imm);
        }
      }
      else if (!strncmp(s, "Jb", 2))
        fprintf(stream, "0x%08x", pc + Mop->fetch.inst.imm + Mop->fetch.inst.len);
      else if (!strncmp(s, "Jw", 2))
        fprintf(stream, "0x%08x", pc + Mop->fetch.inst.imm + Mop->fetch.inst.len);
      else if (!strncmp(s, "Jd", 2))
        fprintf(stream, "0x%08x", pc + Mop->fetch.inst.imm + Mop->fetch.inst.len);
      else if (!strncmp(s, "Jv", 2))
        fprintf(stream, "0x%08x", pc + Mop->fetch.inst.imm + Mop->fetch.inst.len);

      else if (!strncmp(s, "cc", 2))
        fputs(md_cc_names[CC], stream);

      else if (!strncmp(s, "lc", 2))
        fputs(md_lc_names[LC], stream);

      //else
        //panic("unrecognized format string");

      s++;
    }
    else
      fputc(*s, stream);
    s++;
  }
}

/* disassemble an x86 instruction */
  void
md_print_insn(struct Mop_t * Mop,		/* instruction to disassemble */
    FILE *stream)		/* output stream */
{
  enum md_opcode op;

  /* use stderr as default output stream */
  if (!stream)
    stream = stderr;

  /* decode the instruction, assumes predecoded text segment */
  MD_SET_OPCODE(op, Mop->fetch.inst);

  /* disassemble the instruction */
  if (op <= OP_NA || op >= OP_MAX) 
  {
    /* bogus instruction */
    fprintf(stream, "<invalid inst: 0x%08x>, op = %d",
        *(dword_t *)Mop->fetch.inst.code, op);
  }
  else
  {
    switch(Mop->fetch.inst.rep)
    {
      case REP_REP:	fprintf(stream, "rep "); break;
      case REP_REPZ:	fprintf(stream, "repz "); break;
      case REP_REPNZ:	fprintf(stream, "repnz "); break;
    }
    md_print_ifmt(MD_OP_NAME(op), Mop, stream);
    fprintf(stream, "  ");
    md_print_ifmt(MD_OP_FORMAT(op), Mop, stream);
  }
}

/* symbolic register names, parser is case-insensitive */
const struct md_reg_names_t md_reg_names[] =
{
  /* name */	/* file */	/* reg */

  /* integer register file */
  { "EAX",	rt_gpr,		0 },
  { "ECX",	rt_gpr,		1 },
  { "EDX",	rt_gpr,		2 },
  { "EBX",	rt_gpr,		3 },
  { "ESP",	rt_gpr,		4 },
  { "EBP",	rt_gpr,		5 },
  { "ESI",	rt_gpr,		6 },
  { "EDI",	rt_gpr,		7 },
  { "TMP0",	rt_gpr,		8 },
  { "TMP1",	rt_gpr,		9 },
  { "TMP2",	rt_gpr,		10 },
  { "TMP3",	rt_gpr,		11 },
  { "TMP4",	rt_gpr,		12 },
  { "TMP5",	rt_gpr,		13 },
  { "TMP6",	rt_gpr,		14 },
  { "ZERO",	rt_gpr,		15 },

  /* floating point register file - double precision */
  { "ST(0)",	rt_fpr,		0 },
  { "ST(1)",	rt_fpr,		1 },
  { "ST(2)",	rt_fpr,		2 },
  { "ST(3)",	rt_fpr,		3 },
  { "ST(4)",	rt_fpr,		4 },
  { "ST(5)",	rt_fpr,		5 },
  { "ST(6)",	rt_fpr,		6 },
  { "ST(7)",	rt_fpr,		7 },
  { "FTMP0",	rt_fpr,		8 },
  { "FTMP1",	rt_fpr,		9 },
  { "FTMP2",	rt_fpr,		10 },
  { "FTMP3",	rt_fpr,		11 },
  { "FTMP4",	rt_fpr,		12 },
  { "FTMP5",	rt_fpr,		13 },
  { "FTMP6",	rt_fpr,		14 },
  { "FTMP7",	rt_fpr,		15 },

  /* miscellaneous registers */
  { "AFLAGS",	rt_ctrl,	0 },
  { "FSW",	rt_ctrl,	1 },

  /* program counters */
  { "$pc",	rt_PC,		0 },
  { "$npc",	rt_NPC,		0 },

  /* sentinel */
  { NULL,	rt_gpr,		0 }
};

/* returns a register name string */
  char *
md_reg_name(const enum md_reg_type rt, const int reg)
{
  int i;

  for (i=0; md_reg_names[i].str != NULL; i++)
  {
    if (md_reg_names[i].file == rt && md_reg_names[i].reg == reg)
      return md_reg_names[i].str;
  }

  /* no found... */
  return NULL;
}

  char *						/* err str, NULL for no err */
md_reg_obj(struct regs_t *regs,			/* registers to access */
    const int is_write,			/* access type */
    const enum md_reg_type rt,			/* reg bank to probe */
    const int reg,				/* register number */
    struct eval_value_t *val)		/* input, output */
{
  switch (rt)
  {
    case rt_gpr:
      if (reg < 0 || reg >= MD_NUM_IREGS)
        return "register number out of range";

      if (!is_write)
      {
        val->type = et_qword;
        val->value.as_qword = regs->regs_R.dw[reg];
      }
      else
        regs->regs_R.dw[reg] = eval_as_qword(*val);
      break;

    case rt_lpr:
      if (reg < 0 || reg >= MD_NUM_FREGS)
        return "register number out of range";

      if (!is_write)
      {
        val->type = et_qword;
        val->value.as_qword = (qword_t)regs->regs_F.e[reg];
      }
      else
        regs->regs_F.e[reg] = eval_as_qword(*val);
      break;

    case rt_fpr:
      if (reg < 0 || reg >= MD_NUM_FREGS)
        return "register number out of range";

      if (!is_write)
      {
        val->type = et_double;
        val->value.as_double = regs->regs_F.e[reg];
      }
      else
        regs->regs_F.e[reg] = eval_as_double(*val);
      break;

    case rt_PC:
      if (!is_write)
      {
        val->type = et_addr;
        val->value.as_addr = regs->regs_PC;
      }
      else
        regs->regs_PC = eval_as_addr(*val);
      break;

    case rt_NPC:
      if (!is_write)
      {
        val->type = et_addr;
        val->value.as_addr = regs->regs_NPC;
      }
      else
        regs->regs_NPC = eval_as_addr(*val);
      break;

    default:
      panic("bogus register bank");
  }

  /* no error */
  return NULL;
}

/* print integer REG(S) to STREAM */
  void
md_print_ireg(md_gpr_t regs, int reg, FILE *stream)
{
  myfprintf(stream, "%4s: %16d/0x%08x",
      md_reg_name(rt_gpr, reg), regs.w[reg], regs.w[reg]);
}

  void
md_print_iregs(md_gpr_t regs, FILE *stream)
{
  int i;

  for (i=0; i < MD_NUM_IREGS; i += 2)
  {
    md_print_ireg(regs, i, stream);
    fprintf(stream, "  ");
    md_print_ireg(regs, i+1, stream);
    fprintf(stream, "\n");
  }
}

/* print floating point REGS to STREAM */
  void
md_print_fpreg(md_fpr_t regs, int reg, FILE *stream)
{
#if 1
  fprintf(stream, "%4s: %Lf",
      md_reg_name(rt_fpr, reg), regs.e[reg]);
#else
  fprintf(stream, "%4s: %Lf (%016Lx:%08x)",
      md_reg_name(rt_fpr, reg), regs.e[reg],
      E2Qw(regs.e[reg]), E2qW(regs.e[reg]));
#endif
}

  void
md_print_fpregs(md_fpr_t regs, FILE *stream)
{
  int i;

  /* floating point registers */
  for (i=0; i < MD_NUM_FREGS; i += 2)
  {
    md_print_fpreg(regs, i, stream);
    fprintf(stream, "  ");
    md_print_fpreg(regs, i+1, stream);
    fprintf(stream, "\n");
  }
}

#ifndef C0
#define C0            0x0100
#endif
#ifndef C1
#define C1            0x0200
#endif
#ifndef C2
#define C2            0x0400
#endif
#ifndef C3
#define C3            0x4000
#endif

  void
md_print_creg(md_ctrl_t regs, int reg, FILE *stream)
{
  /* index is only used to iterate over these registers, hence no enums... */
  switch (reg)
  {
    case 0:
      myfprintf(stream, "AFLAGS: 0x%08x", regs.aflags);
      fprintf(stream, " {");
      if (regs.aflags & CF)
        fprintf(stream, "CF ");
      if (regs.aflags & PF)
        fprintf(stream, "PF ");
      if (regs.aflags & AF)
        fprintf(stream, "AF ");
      if (regs.aflags & ZF)
        fprintf(stream, "ZF ");
      if (regs.aflags & SF)
        fprintf(stream, "SF ");
      if (regs.aflags & OF)
        fprintf(stream, "OF ");
      if (regs.aflags & DF)
        fprintf(stream, "DF ");
      fprintf(stream, "}");
      break;

    case 1:
      myfprintf(stream, "FSW: 0x%04x", (dword_t)regs.fsw);
      fprintf(stream, " {TOP:%d ", FSW_TOP(regs.fsw));
      if (regs.fsw & C0)
        fprintf(stream, "C0 ");
      if (regs.fsw & C1)
        fprintf(stream, "C1 ");
      if (regs.fsw & C2)
        fprintf(stream, "C2 ");
      if (regs.fsw & C3)
        fprintf(stream, "C3 ");
      fprintf(stream, "}");
      break;

    default:
      panic("bogus control register index");
  }
}

  void
md_print_cregs(md_ctrl_t regs, FILE *stream)
{
  md_print_creg(regs, 0, stream);
  fprintf(stream, "  ");
  md_print_creg(regs, 1, stream);
  fprintf(stream, "  ");
}

/* enum md_opcode -> opcode flags, used by simulators */
const unsigned int md_op2flags[OP_MAX] = {
  NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) CLASS, 
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) CLASS,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NA,
#define CONNECT(OP)
#include "machine.def"
};


  static unsigned long
md_set_decoder(char *name,
    unsigned long mskbits, unsigned long offset,
    enum md_opcode op, unsigned long max_offset)
{
  unsigned long msk_base = mskbits & 0xff;
  unsigned long msk_bound = (mskbits >> 8) & 0xff;
  unsigned long msk;

  if(op == FCOM_Mt)
  {
    /* seems like msk_bound is only computing up to 2 */
    msk_bound = 3; //GL: Hack!
  }

  msk = msk_base;
  do {
    if ((msk + offset) >= MD_MAX_MASK)
      panic("MASK_MAX is too small, inst=`%s', index=%d",
          name, msk + offset);
#ifdef DEBUG
    if (debugging && md_mask2op[msk + offset])
      warn("doubly defined opcode, inst=`%s', index=%d",
          name, msk + offset);
#endif

    md_mask2op[msk + offset] = op;
    msk++;
  } while (msk <= msk_bound);

  return MAX(max_offset, (msk-1) + offset);
}


/* intialize the inst decoder, this function builds the ISA decode tables */
  void
md_init_decoder(void)
{
  unsigned long max_offset = 0;
  unsigned long next_offset = 256;
  unsigned long offset = 0;

#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) \
  max_offset = md_set_decoder(NAME, (MSK), offset, (OP), max_offset);
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
  max_offset = md_set_decoder(#OP, (MSK), offset, (OP), max_offset);
#ifdef DEBUG
#define MD_INIT_DECODER_CHECK(OP)   if (debugging && md_opoffset[OP])					\
                                      warn("doubly defined opoffset, inst=`%s', op=%d, offset=%d",	\
                                           #OP, (int)(OP), offset)
#else
#define MD_INIT_DECODER_CHECK(OP) (void)0
#endif

#define CONNECT(OP)							\
  if ((max_offset+1) > next_offset)					\
  panic("next offset too small");					\
  offset = next_offset;						\
  MD_INIT_DECODER_CHECK(OP);           \
  md_opoffset[OP] = offset;						\
  next_offset = offset + md_opmask[OP] + 1;				\
  if ((md_opmask[OP] & (md_opmask[OP]+1)) != 0)			\
  panic("offset mask is not a power of two 0 1");


#include "machine.def"

#undef MD_INIT_DECODER_CHECK

  if (next_offset >= MD_MAX_MASK)
    panic("MASK_MAX is too small, index==%d", max_offset);
#ifdef DEBUG
  if (debugging)
    info("max offset = %d...", max_offset);
#endif
}


/*
 * microcode field routines
 */
/* XXX */
word_t
md_uop_opc(const enum md_opcode uopcode)
{
  if (((unsigned int)uopcode) >= (1 << 14))
    panic("UOP opcode field overflow: %d", (unsigned int)uopcode);
  return (word_t)uopcode;
}

byte_t
md_uop_reg(const enum md_xfield_t xval, const struct Mop_t * Mop, bool * bogus)
{
  byte_t res;

  switch (xval)
  {
    case XR_AL:		res = MD_REG_AL; break;
    case XR_AH:		res = MD_REG_AH; break;
    case XR_AX:		res = MD_REG_AX; break;
    case XR_EAX:	res = MD_REG_EAX; break;
    case XR_eAX:	res = MD_REG_eAX; break;
    case XR_CL:		res = MD_REG_CL; break;
    case XR_CH:		res = MD_REG_CH; break;
    case XR_CX:		res = MD_REG_CX; break;
    case XR_ECX:	res = MD_REG_ECX; break;
    case XR_eCX:	res = MD_REG_eCX; break;
    case XR_DL:		res = MD_REG_DL; break;
    case XR_DH:		res = MD_REG_DH; break;
    case XR_DX:		res = MD_REG_DX; break;
    case XR_EDX:	res = MD_REG_EDX; break;
    case XR_eDX:	res = MD_REG_eDX; break;
    case XR_BL:		res = MD_REG_BL; break;
    case XR_BH:		res = MD_REG_BH; break;
    case XR_BX:		res = MD_REG_BX; break;
    case XR_EBX:	res = MD_REG_EBX; break;
    case XR_eBX:	res = MD_REG_eBX; break;
    case XR_SP:		res = MD_REG_SP; break;
    case XR_ESP:	res = MD_REG_ESP; break;
    case XR_eSP:	res = MD_REG_eSP; break;
    case XR_BP:		res = MD_REG_BP; break;
    case XR_EBP:	res = MD_REG_EBP; break;
    case XR_eBP:	res = MD_REG_eBP; break;
    case XR_SI:		res = MD_REG_SI; break;
    case XR_ESI:	res = MD_REG_ESI; break;
    case XR_eSI:	res = MD_REG_eSI; break;
    case XR_DI:		res = MD_REG_DI; break;
    case XR_EDI:	res = MD_REG_EDI; break;
    case XR_eDI:	res = MD_REG_eDI; break;

    case XR_TMP0:	res = MD_REG_TMP0; break;
    case XR_TMP1:	res = MD_REG_TMP1; break;
    case XR_TMP2:	res = MD_REG_TMP2; break;
    case XR_ZERO:	res = MD_REG_ZERO; break;

    /* This is an ugly hack; it causes us to return a negative number that
       when passed to DSEG will map it to the uarch hardwired-zero register */
    case XR_SEGNONE: res = _DSEG(MD_REG_ZERO);

    case XR_ST0:	res = MD_REG_ST0; break;
    case XR_ST1:	res = MD_REG_ST1; break;
    case XR_ST2:	res = MD_REG_ST2; break;
    case XR_ST3:	res = MD_REG_ST3; break;
    case XR_ST4:	res = MD_REG_ST4; break;
    case XR_ST5:	res = MD_REG_ST5; break;
    case XR_ST6:	res = MD_REG_ST6; break;
    case XR_ST7:	res = MD_REG_ST7; break;

    case XR_FTMP0:	res = MD_REG_FTMP0; break;
    case XR_FTMP1:	res = MD_REG_FTMP1; break;
    case XR_FTMP2:	res = MD_REG_FTMP2; break;

    case XF_RO:		res = RO; break;
    case XF_R:		res = R; break;
    case XF_RM:		res = RM; break;
    case XF_BASE:	res = (Mop->fetch.inst.base >= 0) ? Mop->fetch.inst.base : MD_REG_ZERO;
                  break;
    case XF_SEG:	res = (Mop->fetch.inst.seg != SEG_DEF) ? Mop->fetch.inst.seg-1 : (byte_t)0;
                  /*fprintf(stderr, "md_uop_reg: %x\n", res);*/
                  break;
    case XF_INDEX:	res = (Mop->fetch.inst.index >= 0) ? Mop->fetch.inst.index : MD_REG_ZERO;
                    break;
    case XF_STI:	res = STI; break;

    case XF_CC:		res = CC; break;

    case XE_CCE:	res = CC_E; break;
    case XE_CCNE:	res = CC_NE; break;
    case XE_FCCB:	res = FCC_B; break;
    case XE_FCCNB:	res = FCC_NB; break;
    case XE_FCCE:	res = FCC_E; break;
    case XE_FCCNE:	res = FCC_NE; break;
    case XE_FCCBE:	res = FCC_BE; break;
    case XE_FCCNBE:	res = FCC_NBE; break;
    case XE_FCCU:	res = FCC_U; break;
    case XE_FCCNU:	res = FCC_NU; break;
    case XE_FNOP:	res = fpstk_nop; break;
    case XE_FPUSH:	res = fpstk_push; break;
    case XE_FPOP:	res = fpstk_pop; break;
    case XE_FPOPPOP:	res = fpstk_poppop; break;
    case XE_FP1:	res = 0; break;

    case XE_ILEN:	res = Mop->fetch.inst.len; break;

    default: if(bogus)
             {
		//fprintf(stdout,"\nbogus in reg");
               	return(MD_REG_EAX);
		//*bogus = TRUE;
               res = 0;
             }
             else
               panic("bogus xfield register specifier: %d", (int)xval);
  }

  if (res > 15)
    panic("register field overflow: %d", (int)xval);
  return res;
}

byte_t
md_uop_immb(const enum md_xfield_t xval, const struct Mop_t * Mop, bool * bogus)
{
  sdword_t res;

  switch (xval)
  {
    case XF_SYSCALL:
      res = 0x80;
      break;

    case XF_DISP:
      res = Mop->fetch.inst.disp;
      break;

    case XF_IMMB:
      res = Mop->fetch.inst.imm;
      break;

    case XE_ILEN:
      res = Mop->fetch.inst.len;
      break;

    case XE_ZERO:
      res = 0;
      break;

    case XE_ONE:
      res = 1;
      break;

    case XE_MONE:
      res = -1;
      break;

    case XE_SIZEV:
      res = ((Mop->fetch.inst.mode & MODE_OPER32) ? 4 : 2);
      break;

    case XE_MSIZEV:
      res = ((Mop->fetch.inst.mode & MODE_OPER32) ? -4 : -2);
      break;

    case XE_CF:		res = CF; break;
    case XE_DF:		res = DF; break;
    case XE_SFZFAFPFCF:	res = (SF|ZF|AF|PF|CF); break;

    default: if(bogus)
             {
               *bogus = TRUE;
               res = 0;
             }
             else
               panic("bogus xfield immediate specifier: %d", (int)xval);
  }

  return res;
}

inline dword_t
md_uop_immv(const enum md_xfield_t xval, const struct Mop_t * Mop, bool * bogus)
{
  dword_t res;

  switch (xval)
  {
    case XF_DISP:
      res = Mop->fetch.inst.disp;
      break;

    case XF_IMMB:
    case XF_IMMV:
    case XF_IMMA:
      res = Mop->fetch.inst.imm;
      break;

    case XE_ILEN:
      res = Mop->fetch.inst.len;
      break;

    case XE_SIZEV_IMMW:
      res = ((Mop->fetch.inst.mode & MODE_OPER32) ? 4 : 2) + Mop->fetch.inst.imm;
      break;

    case XE_ZERO:
      res = 0;
      break;

    case XE_ONE:
      res = 1;
      break;

    case XE_MONE:
      res = (dword_t)-1;
      break;

    case XE_SIZEV:
      res = ((Mop->fetch.inst.mode & MODE_OPER32) ? 4 : 2);
      break;

    case XE_MSIZEV:
      res = ((Mop->fetch.inst.mode & MODE_OPER32) ? -4 : -2);
      break;

    case XE_CF:		res = CF; break;
    case XE_DF:		res = DF; break;
    case XE_SFZFAFPFCF:	res = (SF|ZF|AF|PF|CF); break;

    default: if(bogus)
             {
               *bogus = TRUE;
               res = 0;
             }
             else
               panic("bogus xfield immediate (variable) specifier: %d", (int)xval);
  }
  return res;
}

inline dword_t
md_uop_lit(const enum md_xfield_t xval, const struct Mop_t * Mop, bool * bogus)
{
  dword_t res;

  switch (xval)
  {
    case XE_ZERO:
      res = 0;
      break;

    case XE_ONE:
      res = 1;
      break;

    case XE_MONE:
      res = (dword_t)-1;
      break;

    case XE_SIZEV:
      res = ((Mop->fetch.inst.mode & MODE_OPER32) ? 4 : 2);
      break;

    case XE_MSIZEV:
      res = ((Mop->fetch.inst.mode & MODE_OPER32) ? -4 : -2);
      break;

      //<<<<<<< x86.c
      //case XF_CC:         res = _CC;break;
      //=======
    case XF_CC: 	res = CC; break; // skumar

                  //>>>>>>> 1.3
    case XF_SCALE:	res = Mop->fetch.inst.scale; break;
    case XE_CCE:	res = CC_E; break;
    case XE_CCNE:	res = CC_NE; break;
    case XE_FCCB:	res = FCC_B; break;
    case XE_FCCNB:	res = FCC_NB; break;
    case XE_FCCE:	res = FCC_E; break;
    case XE_FCCNE:	res = FCC_NE; break;
    case XE_FCCBE:	res = FCC_BE; break;
    case XE_FCCNBE:	res = FCC_NBE; break;
    case XE_FCCU:	res = FCC_U; break;
    case XE_FCCNU:	res = FCC_NU; break;
    case XE_FNOP:	res = fpstk_nop; break;
    case XE_FPUSH:	res = fpstk_push; break;
    case XE_FPOP:	res = fpstk_pop; break;
    case XE_FPOPPOP:	res = fpstk_poppop; break;

    default: if(bogus)
             {
               *bogus = TRUE;
               res = 0;
             }
             else
               panic("bogus literal specifier: %d", (int)xval);
  }
  return res;
}

/* UOP code selectors */
#define VMODE32 (Mop->fetch.inst.mode & MODE_OPER32)

#define XV(OP)								\
  ((Mop->fetch.inst.mode & MODE_OPER32) ? SYMCAT(OP,D) : SYMCAT(OP,W))
#define XVI(OP)								\
  ((Mop->fetch.inst.mode & MODE_OPER32) ? SYMCAT(OP,DI) : SYMCAT(OP,WI))

#define XA(OP)								\
  ((Mop->fetch.inst.mode & MODE_ADDR32) ? SYMCAT(OP,D) : SYMCAT(OP,W))
#define XAI(OP)								\
  ((Mop->fetch.inst.mode & MODE_ADDR32) ? SYMCAT(OP,DI) : SYMCAT(OP,WI))
/* concat OP with M and X, where M is D or W depending on the ADDR32 mask, X is size qualifier */
#define XAx(OP,X)								\
  ((Mop->fetch.inst.mode & MODE_ADDR32) ? SYMCAT3(OP,D,X) : SYMCAT3(OP,W,X))

#define XS(OP)								\
  ((Mop->fetch.inst.mode & MODE_STACK32) ? SYMCAT(OP,D) : SYMCAT(OP,W))
#define XSI(OP)								\
  ((Mop->fetch.inst.mode & MODE_STACK32) ? SYMCAT(OP,DI) : SYMCAT(OP,WI))

#define XSXV(OP)								\
  ( (Mop->fetch.inst.mode & MODE_STACK32) ?            \
      ( (Mop->fetch.inst.mode & MODE_OPER32) ? SYMCAT(OP,DD) : SYMCAT(OP,DW)) :            \
      ( (Mop->fetch.inst.mode & MODE_OPER32) ? SYMCAT(OP,WD) : SYMCAT(OP,WW))           \
  )
#define XAXV(OP)								\
  ( (Mop->fetch.inst.mode & MODE_ADDR32) ?            \
      ( (Mop->fetch.inst.mode & MODE_OPER32) ? SYMCAT(OP,DD) : SYMCAT(OP,DW)) :            \
      ( (Mop->fetch.inst.mode & MODE_OPER32) ? SYMCAT(OP,WD) : SYMCAT(OP,WW))           \
  )

/* microcode constructors */
/* FM = fusion mode (fuse to previous uop if user specifies this fusion flag) see machine.h */

#define UOP_R0(UV,OPC,FM)							\
  (flow[i++] = (  ((FM) << 32) \
                | (!!(UV) << 30)			\
                | (md_uop_opc(OPC) << 16)))
#define UOP_R1(UV,OPC,RD,FM)						\
  (flow[i++] = (  ((FM) << 32) \
                | (!!(UV) << 30)						\
                | (md_uop_opc(OPC) << 16)				\
                | (md_uop_reg(RD,Mop,bogus) << 12)))
#define UOP_R2(UV,OPC,RD,RS,FM)						\
  (flow[i++] = (  ((FM) << 32) \
                | (!!(UV) << 30)						\
                | (md_uop_opc(OPC) << 16)			\
                | (md_uop_reg(RD,Mop,bogus) << 12)				\
                | (md_uop_reg(RS,Mop,bogus) << 8)))
#define UOP_R3(UV,OPC,RD,RS,RT,FM)						\
  (flow[i++] = (  ((FM) << 32) \
                | (!!(UV) << 30)						\
                | (md_uop_opc(OPC) << 16)				\
                | (md_uop_reg(RD,Mop,bogus) << 12)				\
                | (md_uop_reg(RS,Mop,bogus) << 8)				\
                | (md_uop_reg(RT,Mop,bogus) << 4)))
#define UOP_R4(UV,OPC,RD,RS,RT,RU,FM)					\
  (flow[i++] = (  ((FM) << 32) \
                | (!!(UV) << 30)						\
                | (md_uop_opc(OPC) << 16)				\
                | (md_uop_reg(RD,Mop,bogus) << 12)				\
                | (md_uop_reg(RS,Mop,bogus) << 8)				\
                | (md_uop_reg(RT,Mop,bogus) << 4)				\
                | md_uop_reg(RU,Mop,bogus)))
#define UOP_RL(UV,OPC,RD,RS,RT,LIT,FM)					\
  (flow[i++] = (  ((FM) << 32) \
                | (!!(UV) << 30)						\
                | (md_uop_opc(OPC) << 16)				\
                | (md_uop_reg(RD,Mop,bogus) << 12)				\
                | (md_uop_reg(RS,Mop,bogus) << 8)				\
                | (md_uop_reg(RT,Mop,bogus) << 4)				\
                | (md_uop_lit(LIT,Mop,bogus))))
#define UOP_RLI(UV,OPC,RD,RB,RI,SC,IV,FM)					\
  (flow[i++] = (  ((FM) << 32) \
                | (1ULL << 31)						\
                | (!!(UV) << 30)					\
                | (md_uop_opc(OPC) << 16)				\
                | (md_uop_reg(RD,Mop,bogus) << 12)				\
                | (md_uop_reg(RB,Mop,bogus) << 8)				\
                | (md_uop_reg(RI,Mop,bogus) << 4)				\
                | (md_uop_lit(SC,Mop,bogus))),				\
   flow[i++] = md_uop_immv(IV,Mop,bogus)),                                  \
   flow[i++] = 0
// Address computation with segment override - UCSD
#define UOP_RLI_OV(UV,OPC,RD,SG,RB,RI,SC,IV,FM)				\
  (flow[i++] = (  ((FM) << 32) \
                | (1ULL << 31)						\
                | (!!(UV) << 30)					\
                | (md_uop_opc(OPC) << 16)				\
                | (md_uop_reg(RD,Mop,bogus) << 12)				\
                | (md_uop_reg(RB,Mop,bogus) << 8)				\
                | (md_uop_reg(RI,Mop,bogus) << 4)				\
                | (md_uop_lit(SC,Mop,bogus))),				\
   flow[i++] = md_uop_immv(IV,Mop,bogus)),                                  \
   flow[i++] = md_uop_reg(SG,Mop,bogus)
#define UOP_IB(UV,OPC,RD,RS,IB,FM)						\
  (flow[i++] = (  ((FM) << 32) \
                | (!!(UV) << 30)						\
                | (md_uop_opc(OPC) << 16)				\
                | (md_uop_reg(RD,Mop,bogus) << 12)				\
                | (md_uop_reg(RS,Mop,bogus) << 8)				\
                | (md_uop_immb(IB,Mop,bogus))))
#define UOP_IV(UV,OPC,RD,RS,IV,FM)						\
  (flow[i++] = (  ((FM) << 32) \
                | (1ULL << 31)						\
                | (!!(UV) << 30)					\
                | (md_uop_opc(OPC) << 16)				\
                | (md_uop_reg(RD,Mop,bogus) << 12)				\
                | (md_uop_reg(RS,Mop,bogus) << 8)),				\
   flow[i++] = md_uop_immv(IV,Mop,bogus)),                                  \
   flow[i++] = 0

// Note FP-stack modifications
#define FP_STACK_OP(op) \
  Mop->decode.fpstack_op = (op)

/* UOP flow generator, returns a small non-cyclic program implementing OP,
   returns length of flow returned */
int
md_get_flow(struct Mop_t * Mop, uop_inst_t flow[MD_MAX_FLOWLEN], bool * bogus)
{
  int i = 0;

  switch (Mop->decode.op)
  {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) \
    case OP:								\
                            OP##_FLOW;                                                        \
    break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
    case OP:							\
                          panic("attempted to decode a linking opcode");
#define CONNECT(OP)
#include "machine.def"
    default: if(bogus)
             {
               *bogus = TRUE;
               i = 0;
             }
             else
               panic("bogus opcode");
  }
  return i;
}

/* enum md_opcode -> description string */
const char *md_uop2name[OP_MAX] = {
  NULL, /* NA */
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) NAME,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NAME,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> opcode operand format, used by disassembler */
#define MD_UOP_FORMAT(OP)	(md_uop2format[OP])

/* enum md_opcode -> opcode operand format, used by disassembler */
const char *md_uop2format[OP_MAX] = {
  NULL, /* NA */
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) OPFORM,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NULL,
#define CONNECT(OP)
#include "machine.def"
};

/* generate an instruction format string */
  void
md_print_ufmt(const char *fmt,
    struct uop_t *uop,		/* instruction to disassemble */
    md_addr_t pc,             /* addr of inst, used for PC-rels */
    FILE *stream)
{
  const char *s = fmt;

  while (*s)
  {
    if (*s == '%')
    {
      s++;

      if (!strncmp(s, "Db", 2))
        fputs(md_gprb_names[URD], stream);
      else if (!strncmp(s, "Sb", 2))
        fputs(md_gprb_names[URS], stream);
      else if (!strncmp(s, "Tb", 2))
        fputs(md_gprb_names[URT], stream);
      else if (!strncmp(s, "Ub", 2))
        fputs(md_gprb_names[URU], stream);

      else if (!strncmp(s, "Dw", 2))
        fputs(md_gprw_names[URD], stream);
      else if (!strncmp(s, "Sw", 2))
        fputs(md_gprw_names[URS], stream);
      else if (!strncmp(s, "Tw", 2))
        fputs(md_gprw_names[URT], stream);
      else if (!strncmp(s, "Uw", 2))
        fputs(md_gprw_names[URU], stream);

      else if (!strncmp(s, "Dd", 2))
        fputs(md_gprd_names[URD], stream);
      else if (!strncmp(s, "Sd", 2))
        fputs(md_gprd_names[URS], stream);
      else if (!strncmp(s, "Td", 2))
        fputs(md_gprd_names[URT], stream);
      else if (!strncmp(s, "Ud", 2))
        fputs(md_gprd_names[URU], stream);

      else if (!strncmp(s, "De", 2))
        fputs(md_fpre_names[URD], stream);
      else if (!strncmp(s, "Se", 2))
        fputs(md_fpre_names[URS], stream);
      else if (!strncmp(s, "Te", 2))
        fputs(md_fpre_names[URT], stream);

      else if (!strncmp(s, "st", 2))
        fputs(md_fpstk_names[ULIT], stream);

      else if (!strncmp(s, "Ib", 2))
        fprintf(stream, "0x%02x", UIMMB);
      else if (!strncmp(s, "Iw", 2))
        fprintf(stream, "0x%04x", UIMMW);
      else if (!strncmp(s, "Id", 2))
        fprintf(stream, "0x%08x", UIMMD);

      else if (!strncmp(s, "Jb", 2))
        fprintf(stream, "0x%08llx", pc + UIMMB + URS);
      else if (!strncmp(s, "Jw", 2))
        fprintf(stream, "0x%08llx", pc + UIMMW + URS);
      else if (!strncmp(s, "Jd", 2))
        fprintf(stream, "0x%08llx", pc + UIMMD + URS);

      else if (!strncmp(s, "Kb", 2))
        fprintf(stream, "0x%08llx", pc + UIMMB + URT);
      else if (!strncmp(s, "Kw", 2))
        fprintf(stream, "0x%08llx", pc + UIMMW + URT);
      else if (!strncmp(s, "Kd", 2))
        fprintf(stream, "0x%08llx", pc + UIMMD + URT);

      else if (!strncmp(s, "Li", 2))
        fprintf(stream, "%d", (unsigned int)ULIT);
      else if (!strncmp(s, "cc", 2))
        fputs(md_cc_names[UCC], stream);
      else if (!strncmp(s, "lc", 2))
        fputs(md_lc_names[URS], stream);

      else
        panic("unrecognized format string");

      s++;
    }
    else
      fputc(*s, stream);
    s++;
  }
}

/* FIX uop print function to align with alpha/pisa/arm */
/* print uop function */
#if 1
void
md_print_uop(const enum md_opcode op,
    const md_inst_t instruction,	/* instruction to disassemble */
    const md_addr_t pc,		/* addr of inst, used for PC-rels */
    FILE *stream){}
#else
/* #ifdef FIXME */
  void
md_print_uop(const enum md_opcode uop,
    const md_inst_t instruction,	/* instruction to disassemble */
    const md_addr_t pc,		/* addr of inst, used for PC-rels */
    FILE *stream)		/* output stream */
{
  enum md_uopcode_t uopcode;

  /* use stderr as default output stream */
  if (!stream)
    stream = stderr;

  /* decode the instruction, assumes predecoded text segment */
  MD_SET_UOPCODE(uopcode, uop);

  /* disassemble the instruction */
  if (uopcode <= UOP_NA || uopcode >= UOP_MAX) 
  {
    /* bogus instruction */
    fprintf(stream, "<invalid inst: 0x%08x>, op = %d",
        *(dword_t *)uop, uopcode);
  }
  else
  {
    md_print_ufmt(MD_UOP_NAME(uopcode), uop, pc, stream);
    fprintf(stream, "  ");
    md_print_ufmt(MD_UOP_FORMAT(uopcode), uop, pc, stream);
  }
}
#endif

  void
md_print_uop_1(struct uop_t *uop,
    md_inst_t instruction,	/* instruction to disassemble */
    md_addr_t pc,		/* addr of inst, used for PC-rels */
    FILE *stream)		/* output stream */
{
  enum md_opcode uopcode;

  /* use stderr as default output stream */
  if (!stream)
    stream = stderr;

  /* decode the instruction, assumes predecoded text segment */
  MD_SET_UOPCODE(uopcode, &uop->decode.raw_op);

  /* disassemble the instruction */
  if (uopcode <= OP_NA || uopcode >= OP_MAX) 
  {
    /* bogus instruction */
    fprintf(stream, "<invalid inst: 0x%08x>, op = %d", *(dword_t *)(&uop->decode.raw_op), uopcode);
  }
  else
  {
    md_print_ufmt(MD_OP_NAME(uopcode), uop, pc, stream);
    fprintf(stream, "  ");
    md_print_ufmt(MD_OP_FORMAT(uopcode), uop, pc, stream);
  }
}


#undef R0
#undef R
#undef RM
#undef C0
#undef C1
#undef C2
#undef C3

