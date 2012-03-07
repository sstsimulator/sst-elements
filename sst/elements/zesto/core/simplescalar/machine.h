/*
 * x86.h - x86 ISA definitions
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 * 
 * Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
 *
 * This source file is distributed "as is" in the hope that it will be
 * useful.  The tool set comes with no warranty, and no author or
 * distributor accepts any responsibility for the consequences of its
 * use. 
 * 
 * Everyone is granted permission to copy, modify and redistribute
 * this tool set under the following conditions:
 * 
 *    This source code is distributed for non-commercial use only. 
 *    Please contact the maintainer for restrictions applying to 
 *    commercial use.
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 */

#ifndef X86_H
#define X86_H

#ifdef __cplusplus
extern "C" {
#else
#define bool int
#endif

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include <stdint.h>

#include "host.h"
#include "misc.h"


/*
 * This file contains various definitions needed to decode, disassemble, and
 * execute x86 instructions.
 */

/* build for x86 target */
#define TARGET_X86
#define MAX_CORES 64


/* probe cross-endian execution */
#if defined(BYTES_BIG_ENDIAN)
#define MD_CROSS_ENDIAN
#endif

/* not applicable/available, usable in most definition contexts */
#define NA            0
#define DNA             -1

/*
 * target-dependent type definitions
 */

/* define MD_QWORD_ADDRS if the target requires 64-bit (qword) addresses */
/* #define MD_QWORD_ADDRS */

/* address type definition (32-bit) */
typedef dword_t md_addr_t;

/* physical address type definition (64-bit) */
typedef qword_t md_paddr_t;


/*
 * target-dependent memory module configuration
 */

/* physical memory page size (must be a power-of-two) */
#define MD_PAGE_SIZE        4096
#define MD_LOG_PAGE_SIZE    12

/* convert 64-bit inst text addresses to 32-bit inst equivalents */
#define IACOMPRESS(A)        (A)
#define ISCOMPRESS(SZ)        (SZ)

/*
 * target-dependent instruction faults
 */

enum md_fault_type {
  md_fault_none = 0,        /* no fault */
  md_fault_access,        /* storage access fault */
  md_fault_alignment,        /* storage alignment fault */
  md_fault_overflow,        /* signed arithmetic overflow fault */
  md_fault_div0,        /* division by zero fault */
  md_fault_invalid,             /* invalid arithmetic operation */ 
                                /* added to allow SQRT{S,T} in FIX exts */
  md_fault_break,        /* BREAK instruction fault */
  md_fault_unimpl,        /* unimplemented instruction fault */
  md_fault_internal        /* internal S/W fault */
};


/*
 * target-dependent register file definitions, used by regs.[hc]
 */

/* number of integer registers */
#define MD_NUM_IREGS        (/* arch */8 + /* uarch */8)
#define MD_NUM_ARCH_IREGS        8

/* number of floating point registers */
#define MD_NUM_FREGS        (/* arch */8 + /* uarch */8)

/* number of control registers */
#define MD_NUM_CREGS        3

/* number of segment registers */
#define MD_NUM_SREGS        6 // UCSD

/* total number of registers, excluding PC and NPC */
#define MD_TOTAL_REGS                            \
  (/*int*/MD_NUM_IREGS + /*fp*/MD_NUM_FREGS + /*misc*/MD_NUM_CREGS + /*seg*/MD_NUM_SREGS )

#define GPR_OFFSET 0
#define FPR_OFFSET MD_NUM_IREGS
#define CREG_OFFSET (FPR_OFFSET+MD_NUM_FREGS)
#define SEG_OFFSET (CREG_OFFSET+MD_NUM_CREGS)

#define REG_IS_GPR(N) (((N) >= GPR_OFFSET) && ((N) < FPR_OFFSET))
#define REG_IS_FPR(N) (((N) >= FPR_OFFSET) && ((N) < CREG_OFFSET))
#define REG_IS_CREG(N) (((N) >= CREG_OFFSET) && ((N) < SEG_OFFSET))
#define REG_IS_SEG(N) (((N) >= SEG_OFFSET) && ((N) < MD_TOTAL_REGS))

/* these macros map the architected register number to a unique number */
/* NOTE: DFPR "renames" the FP regs based on the FP stack */
#define DGPR(N) ((N)+GPR_OFFSET)
#define DFPR(N) ((F2P(N))+FPR_OFFSET)
#define DCREG(N) ((N)+CREG_OFFSET)
#define DSEG(N) ((N)+SEG_OFFSET)

#define DGPR_B(N) ((_ARCH(N) ? (_ID(N)) : (N)) + GPR_OFFSET)
#define DGPR_W(N) (N+GPR_OFFSET)
#define DGPR_D(N) (N+GPR_OFFSET)

/* these macros undo the mapping back to the original ISA register numbers */
#define _DGPR(N) ((N)-GPR_OFFSET)
#define _DFPR(N) ((N)-FPR_OFFSET)
#define _DCREG(N) ((N)-CREG_OFFSET)
#define _DSEG(N) ((N)-SEG_OFFSET)

/* general purpose (integer) register file entry type */
typedef union {
  dword_t dw[MD_NUM_IREGS];
  struct { word_t lo; word_t hi; } w[MD_NUM_IREGS];
  struct { byte_t lo; byte_t hi; word_t pad; } b[MD_NUM_IREGS];
} md_gpr_t;

/* floating point register file entry type */
typedef union {
  efloat_t e[MD_NUM_FREGS];    /* extended-precision floating point view */
} md_fpr_t;

/* segment register file entry type */
typedef union{
  word_t w[MD_NUM_SREGS]; // UCSD
} md_seg_t;

/* control register file contents */
typedef struct {
  dword_t aflags;        /* processor arithmetic flags */
  word_t cwd;            /* floating point control word */
  word_t fsw;            /* floating point status register */
} md_ctrl_t;

/* well known registers */
enum md_reg_names {
  MD_REG_AL = 0,
  MD_REG_AX = 0,
  MD_REG_EAX = 0,
  MD_REG_eAX = 0,

  MD_REG_CL = 1,
  MD_REG_CX = 1,
  MD_REG_ECX = 1,
  MD_REG_eCX = 1,

  MD_REG_DL = 2,
  MD_REG_DX = 2,
  MD_REG_EDX = 2,
  MD_REG_eDX = 2,

  MD_REG_BL = 3,
  MD_REG_BX = 3,
  MD_REG_EBX = 3,
  MD_REG_eBX = 3,

  MD_REG_AH = 4,
  MD_REG_SP = 4,    /* stack pointer */
  MD_REG_ESP = 4,
  MD_REG_eSP = 4,

  MD_REG_CH = 5,    /* base pointer */
  MD_REG_BP = 5,
  MD_REG_EBP = 5,
  MD_REG_eBP = 5,

  MD_REG_DH = 6,
  MD_REG_SI = 6,
  MD_REG_ESI = 6,
  MD_REG_eSI = 6,

  MD_REG_BH = 7,
  MD_REG_DI = 7,
  MD_REG_EDI = 7,
  MD_REG_eDI = 7,

  MD_REG_TMP0 = 8,
  MD_REG_TMP1 = 9,
  MD_REG_TMP2 = 10,

  MD_REG_ZERO = 15

};

enum md_freg_names {
  MD_REG_ST0 = 0,
  MD_REG_ST1 = 1,
  MD_REG_ST2 = 2,
  MD_REG_ST3 = 3,
  MD_REG_ST4 = 4,
  MD_REG_ST5 = 5,
  MD_REG_ST6 = 6,
  MD_REG_ST7 = 7,

  MD_REG_FTMP0 = 8,
  MD_REG_FTMP1 = 9,
  MD_REG_FTMP2 = 10
};

/* UCSD */
enum md_sreg_names {
  MD_REG_CS = 0, 
  MD_REG_SS = 1, 
  MD_REG_DS = 2, 
  MD_REG_ES = 3, 
  MD_REG_FS = 4, 
  MD_REG_GS = 5 
};

/* GL */
enum md_creg_names {
  MD_REG_AFLAGS = 0, 
  MD_REG_CWD = 1, 
  MD_REG_FSW = 2
};

/* Not used by x86, present for ARM compatibility */
#define MD_REG_PC       0

/*
 * target-dependent instruction format definition
 */

/* maximum instruction length in bytes */
#define MD_MAX_ILEN        16


/*QEMU can give multiple mem-ops if the instruction has multiple references instructions can have loads and stores in the same macro-op
 eg a MOVS instruction gets converted into a store and load micro-op */
typedef struct mem_ops{
md_paddr_t mem_vaddr_ld[2];
md_paddr_t mem_paddr_ld[2];
bool ld_dequeued[2];
byte_t ld_size[2];

md_paddr_t mem_vaddr_str[2];
md_paddr_t mem_paddr_str[2]; 
bool str_dequeued[2];
byte_t str_size[2];

unsigned int memops;	//The number of memory ops dequeued
}mem_ops_t;


/* instruction formats */
typedef struct md_inst_t {

  /* instruction code */

  byte_t code[MD_MAX_ILEN];

  /* operational mode information */

  byte_t npfx;            /* number of prefix bytes */

#define MODE_OPER32        0x0001
#define MODE_ADDR32        0x0002
#define MODE_STACK32        0x0004
  byte_t mode;            /* execution mode prefix */

#define REP_NONE        0
#define REP_REPNZ       1
#define REP_REPZ        2
#define REP_REP         3

  byte_t rep;            /* repeat mode */

  byte_t lock;            /* locked? */

#define SEG_DEF            0 /* default */
#define SEG_CS            1
#define SEG_SS            2
#define SEG_DS            3
#define SEG_ES            4
#define SEG_FS            5
#define SEG_GS            6
  byte_t seg;            /* segment override */ /* UCSD - Verify */

  /* predecode information */

  byte_t nopc;            /* number of opcode bytes */
  byte_t modrm;            /* has modrm byte? (0/1) */

  byte_t sib;            /* has sib byte? (0/1) */
  sbyte_t base;            /* base register (-1 == none) */
  sbyte_t index;        /* index register (-1 == none) */

#define SCALE_1            0
#define SCALE_2            1
#define SCALE_4            2
#define SCALE_8            3
  byte_t scale;            /* index scale factor */

  byte_t ndisp;            /* size of displacement in bytes */
  sdword_t disp;            /* sign-extended displacement value */

  byte_t nimm;            /* size of displacement in bytes */
  sdword_t imm;            /* sign-extended immediate value */

  byte_t len;            /* inst length in bytes */

  byte_t qemu_len;	/* inst length given by QEMU We observe for some instruction that QEMU lengths and the one decoded by our decoder our different	*/
  
  md_addr_t vaddr;	/* virtual address of the instruction */

  md_paddr_t paddr;	/* physical address of the instruction */

  mem_ops_t mem_ops;    /*stores the memory ops of the instruction*/

  md_paddr_t mem_vaddr; /* virtual address of memory access */

  md_paddr_t mem_paddr; /* physical address of memory access */

  byte_t size;		/* size of the memory access, is zero if its not a memory Op */

} md_inst_t;

enum MemOpType { MEM_RD = 0, MEM_WR=1 };

/* XXX */
/* preferred nop instruction definition */
extern const md_inst_t MD_NOP_INST;

extern unsigned long long timestamp[MAX_CORES]; // for RDTSC
extern int is_syscall;


/* target swap support */
#ifdef MD_CROSS_ENDIAN

#define MD_SWAPW(X)        SWAP_WORD(X)
#define MD_SWAPD(X)        SWAP_DWORD(X)
#define MD_SWAPI(X)        SWAP_DWORD(X)
#define MD_SWAPQ(X)        SWAP_QWORD(X)

#else /* !MD_CROSS_ENDIAN */

#define MD_SWAPW(X)        (X)
#define MD_SWAPD(X)        (X)
#define MD_SWAPI(X)        (X)
#define MD_SWAPQ(X)        (X)

#endif


/* get inst size */
#define MD_INST_SIZE(INST)              (INST).len

/*
 * target-dependent loader module configuration
 */

/* maximum size of argc+argv+envp environment */
#define MD_MAX_ENVIRON        16384
//#define MD_MAX_ENVIRON     8388608        // as given by the limit command

/*
 * machine.def specific definitions
 */

/* global opcode names, these are returned by the decoder (MD_OP_ENUM()) */

enum md_opcode {
  OP_NA = 0,    /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) OP,
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,CLASS,O1,I1,I2,I3,OFLAGS,IFLAGS) OP,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) OP,
#define CONNECT(OP)
#include "machine.def"
  OP_MAX    /* number of opcodes + NA */
};

enum md_opcode md_decode(const byte_t mode, md_inst_t *inst, const int set_nop);

/* inst -> enum md_opcode mapping, use this macro to decode insts */
#define MD_SET_OPCODE(OP, INST)                        \
  (OP) = md_decode(MODE_OPER32|MODE_ADDR32|MODE_STACK32, &INST, 0)

#define MD_SET_OPCODE_DURING_FETCH(OP, INST)                        \
  (OP) = md_decode(MODE_OPER32|MODE_ADDR32|MODE_STACK32, &INST, 1)

#define MD_GET_UOP(FLOW,INDEX)                       \
  ((uop = &FLOW[INDEX]),MD_SET_UOPCODE(op,uop))

#define MD_CHECK_PRED(OP,INST)    TRUE

#define MD_INC_FLOW       (UHASIMM ? 3 : 1)

/* largest opcode field value (currently upper 8-bit are used for pre/post-
    incr/decr operation specifiers */
#define MD_MAX_MASK        2048

/* internal decoder state */

extern enum md_opcode md_mask2op[];
extern unsigned int md_opoffset[];
extern const unsigned int md_opmask[];
extern const unsigned int md_opshift[];


/* enum md_opcode -> description string */
#define MD_OP_NAME(OP)        (md_op2name[OP])
extern const char *md_op2name[];

/* enum md_opcode -> enum name string */
#define MD_OP_ENUM(OP)        (md_op2enum[OP])
extern const char *md_op2enum[];

/* enum md_opcode -> opcode operand format, used by disassembler */
#define MD_OP_FORMAT(OP)    (md_op2format[OP])
extern const char *md_op2format[];

/* function unit classes, update md_fu2name if you update this definition */
enum md_fu_class {
  FU_INVALID = -1,  /* inst cannot use a functional unit (assigned to Macro-Ops who
                       shouldn't be directly executed... only uops are assigned valid FU's) */
  FU_NA = 0,        /* inst does not use a functional unit */
  FU_IEU,           /* integer ALU */
  FU_JEU,           /* jump execution unit */
  FU_IMUL,          /* integer multiplier  */
  FU_IDIV,          /* integer divider */
  FU_SHIFT,         /* integer shifter */
  FU_FADD,          /* floating point adder */
  FU_FMUL,          /* floating point multiplier */
  FU_FDIV,          /* floating point divider */
  FU_FCPLX,         /* floating point complex (sqrt,transcendentals,...) */
  FU_LD,            /* load port/AGU */
  FU_STA,           /* store-address port/AGU */
  FU_STD,           /* store-data port */
  NUM_FU_CLASSES    /* total functional unit classes */
};

/* enum md_opcode -> enum md_fu_class, used by performance simulators */
#define MD_OP_FUCLASS(OP)    (md_op2fu[OP])
extern const enum md_fu_class md_op2fu[];

/* enum md_fu_class -> description string */
#define MD_FU_NAME(FU)        (md_fu2name[FU])
extern const char *md_fu2name[];

/* instruction flags */
#define F_ICOMP        0x00000001    /* integer computation */
#define F_FCOMP        0x00000002    /* FP computation */
#define F_CTRL        0x00000004    /* control inst */
#define F_UNCOND    0x00000008    /*   unconditional change */
#define F_COND        0x00000010    /*   conditional change */
#define F_MEM        0x00000020    /* memory access inst */
#define F_LOAD        0x00000040    /*   load inst */
#define F_STORE        0x00000080    /*   store inst */
#define F_DISP        0x00000100    /*   displaced (R+C) addr mode */
#define F_RR        0x00000200    /*   R+R addr mode */
#define F_DIRECT    0x00000400    /*   direct addressing mode */
#define F_TRAP        0x00000800    /* traping inst */
#define F_LONGLAT    0x00001000    /* long latency inst (for sched) */
#define F_DIRJMP    0x00002000    /* direct jump */
#define F_INDIRJMP    0x00004000    /* indirect jump */
#define F_CALL        0x00008000    /* function call */
#define F_FPCOND    0x00010000    /* FP conditional branch */
#define F_IMM        0x00020000    /* instruction has immediate operand */
#define F_CISC        0x00040000    /* CISC instruction */
#define F_AGEN        0x00080000    /* AGEN micro-instruction */
#define F_IMMB        0x00100000    /* inst has 1-byte immediate */
#define F_IMMW        0x00200000    /* inst has 2-byte immediate */
#define F_IMMD        0x00300000    /* inst has 4-byte immediate */
#define F_IMMV        0x00400000    /* inst has 1-byte immediate */
#define F_IMMA        0x00500000    /* inst has 1-byte immediate */
#define F_UIMM        0x01000000    /* immediate operand is unsigned */
#define F_NOMOD        0x02000000    /* no modrm byte */
#define F_REP        0x04000000    /* no zero check during repeat */
#define F_UCODE         0x08000000      /* instruction produces ucode  */
#define F_RTS           0x10000000      /* run-time support instruction */
#define F_SPEC          0x20000000      /* speculative instruction (hint) */
#define F_RETN          0x40000000      /* subroutine return */
#define F_FENCE		0x80000000	/* Memory fence instruction */
/* non-zero if instruction has an immediate operand */
#define F_HASIMM    0x00f00000

/* returns size of immediate operand */
#define F_IMMSZ(M,X)    (((((X) >> 20) & 0xf) == 0)            \
    ? 0                        \
    : (((((X) >> 20) & 0xf) == 1)            \
      ? 1                        \
      : (((((X) >> 20) & 0xf) == 2)        \
        ? 2                    \
        : (((((X) >> 20) & 0xf) == 3)        \
          ? 4                    \
          : (((((X) >> 20) & 0xf) == 4)        \
            ? (((M) & MODE_OPER32) ? 4 : 2)    \
            : (((((X) >> 20) & 0xf) == 5)    \
              ? (((M) & MODE_ADDR32) ? 4 : 2)    \
              : (abort(), 0)))))))

/* enum md_opcode -> opcode flags, used by simulators */
#define MD_OP_FLAGS(OP)        (md_op2flags[OP])
extern const unsigned int md_op2flags[];


/* integer register specifiers */
#define RO        (Mop->fetch.inst.code[Mop->fetch.inst.npfx + Mop->fetch.inst.nopc - 1] & 0x07)
#define R        ((Mop->fetch.inst.code[Mop->fetch.inst.npfx + Mop->fetch.inst.nopc] >> 3) & 0x07)
#define RM        (Mop->fetch.inst.code[Mop->fetch.inst.npfx + Mop->fetch.inst.nopc] & 0x07)

/* floating point register specifiers */
#define STI        (Mop->fetch.inst.code[Mop->fetch.inst.npfx + Mop->fetch.inst.nopc] & 0x07)

/* Segment override - UCSD */
#define SEG_INDEX                                                      \
  Mop->fetch.inst.seg-1

#define SEG                                                            \
  ((Mop->fetch.inst.seg != SEG_DEF)                                              \
   ?  SEG_W(SEG_INDEX)                                                   \
   : (word_t)0)                                                                      

/* addressing mode fields */
#define BASE                                \
  ((Mop->fetch.inst.base >= 0)                            \
   ? ((Mop->fetch.inst.mode & MODE_ADDR32) ? GPR_D(Mop->fetch.inst.base) : GPR_W(Mop->fetch.inst.base))    \
   : 0)
#define INDEX                                \
  ((Mop->fetch.inst.index >= 0)                            \
   ? ((Mop->fetch.inst.mode & MODE_ADDR32) ? GPR_D(Mop->fetch.inst.index) : GPR_W(Mop->fetch.inst.index))\
   : 0)
#define SCALE        (Mop->fetch.inst.scale)
#define DISP        (Mop->fetch.inst.disp)

/* address generation */
/* Ganesh */
#define AGEN_W(S,B,I,SC,D)                        \
  ((Mop->fetch.inst.seg == SEG_DEF)                                                  \
   ? ((word_t)((dword_t)(B) + ((dword_t)(I) << (SC)) + (dword_t)(D)))        \
   : ((word_t)((dword_t)(B) + ((dword_t)(thread->ldt_p?((thread->ldt_p)[S>>3]):0)) + ((dword_t)(I) << (SC)) + (dword_t)(D))))

#define AGEN_D(S,B,I,SC,D)                                            \
  ((Mop->fetch.inst.seg == SEG_DEF)                                                  \
   ? ((dword_t)((dword_t)(B) + ((dword_t)(I) << (SC)) + (dword_t)(D)))        \
   : ((dword_t)((dword_t)(B) + ((dword_t)(thread->ldt_p?((thread->ldt_p)[S>>3]):0)) + ((dword_t)(I) << (SC)) + (dword_t)(D))))

/*  ((dword_t)((dword_t)(B) + ((dword_t)(I) << (SC)) + (dword_t)(D)))*/

#if 1
#define AGEN_A(S,B,I,SC,D)                        \
  ((Mop->fetch.inst.mode & MODE_ADDR32) ? AGEN_D(S,B,I,SC,D) : AGEN_W(S,B,I,SC,D))
#define AGEN_S(S,B,I,SC,D)                        \
  ((Mop->fetch.inst.mode & MODE_STACK32) ? AGEN_D(S,B,I,SC,D) : AGEN_W(S,B,I,SC,D))
#else
#define AGEN_A(S,B,I,SC,D)                        \
  /* FIXME: ignoring the SEG */                        \
((Mop->fetch.inst.mode & MODE_ADDR32)                        \
 ? (dword_t)(/*(word_t)(S) +*/ (word_t)0 + (dword_t)(B) + ((dword_t)(I) << (SC)) + (dword_t)(D))    \
 : (word_t)(/* (word_t)(S) +*/(word_t)0 +  (dword_t)(B) + ((dword_t)(I) << (SC)) + (dword_t)(D)))

#define AGEN_S(S,B,I,SC,D)                        \
  /* FIXME: ignoring the SEG */                        \
((Mop->fetch.inst.mode & MODE_STACK32)                        \
 ? (dword_t)(/*(word_t)(S) + */ (dword_t)(B) + ((dword_t)(I) << (SC)) + (dword_t)(D))    \
 : (word_t)(/*(word_t)(S) +*/ (dword_t)(B) + ((dword_t)(I) << (SC)) + (dword_t)(D)))
#endif

/* modrm field accessors */
#define MODRM_MOD(X)        (((X) >> 6) & 0x03)
#define MODRM_R(X)        (((X) >> 3) & 0x07)
#define MODRM_OPC(X)        (((X) >> 3) & 0x07)
#define MODRM_RM(X)        ((X) & 0x07)

/* sib byte field accessors */
#define SIB_SCALE(X)        (((X) >> 6) & 0x03)
#define SIB_INDEX(X)        (((X) >> 3) & 0x07)
#define SIB_BASE(X)        ((X) & 0x07)

/* miscellaneous fields */
#define CC        (Mop->fetch.inst.code[Mop->fetch.inst.npfx + Mop->fetch.inst.nopc - 1] & 0x0f)

/* FP constant field */
#define LC        (Mop->fetch.inst.code[Mop->fetch.inst.npfx + Mop->fetch.inst.nopc] & 0x07)

/* immediates */
#define IMM_B        (assert(Mop->fetch.inst.nimm == 1), Mop->fetch.inst.imm)
#define IMM_W        (assert(Mop->fetch.inst.nimm == 2), Mop->fetch.inst.imm)
#define IMM_D        (assert(Mop->fetch.inst.nimm == 4), Mop->fetch.inst.imm)
#define IMM_V        ((Mop->fetch.inst.mode & MODE_OPER32)            \
    ? (assert(Mop->fetch.inst.nimm == 4), Mop->fetch.inst.imm)        \
    : (assert(Mop->fetch.inst.nimm == 2), Mop->fetch.inst.imm))
#define IMM_A        ((Mop->fetch.inst.mode & MODE_ADDR32)            \
    ? (assert(Mop->fetch.inst.nimm == 4), Mop->fetch.inst.imm)        \
    : (assert(Mop->fetch.inst.nimm == 2), Mop->fetch.inst.imm))

#define UIMM_B        (assert(Mop->fetch.inst.nimm == 1), (dword_t)(Mop->fetch.inst.imm & 0xff))
#define UIMM_W        (assert(Mop->fetch.inst.nimm == 2), (dword_t)(Mop->fetch.inst.imm & 0xffff))
#define UIMM_D        (assert(Mop->fetch.inst.nimm == 4), (dword_t)Mop->fetch.inst.imm)
#define UIMM_V        ((Mop->fetch.inst.mode & MODE_OPER32)            \
    ? (assert(Mop->fetch.inst.nimm == 4), (dword_t)Mop->fetch.inst.imm)    \
    : (assert(Mop->fetch.inst.nimm == 2), (dword_t)Mop->fetch.inst.imm))
#define UIMM_A        ((Mop->fetch.inst.mode & MODE_ADDR32)            \
    ? (assert(Mop->fetch.inst.nimm == 4), (dword_t)Mop->fetch.inst.imm)    \
    : (assert(Mop->fetch.inst.nimm == 2), (dword_t)Mop->fetch.inst.imm))

/* load/store 12-bit unsigned offset field value */
#define OFS        ((dword_t)(Mop->fetch.inst & 0xfff))

/* load/store 8-bit unsigned offset field value */
#define HOFS        ((dword_t)((RS << 4) + RM))

/* fp load/store 8-bit unsigned offset field value */
#define FPOFS        ((dword_t)((Mop->fetch.inst & 0xff) << 2))

/* returns 24-bit signed immediate field value - made 2's complement - Onur 07/24/00 */
#define BOFS                                \
  ((Mop->fetch.inst & 0x800000)                            \
   ? (0xfc000000 | ((Mop->fetch.inst & 0xffffff) << 2))                \
   : ((Mop->fetch.inst & 0x7fffff) << 2))

#define IS_EVEN_ONES(r) \
  (((r&0x1)+    \
    ((r>>1)&0x1)+ \
    ((r>>2)&0x1)+ \
    ((r>>3)&0x1)+ \
    ((r>>4)&0x1)+ \
    ((r>>5)&0x1)+ \
    ((r>>6)&0x1)+ \
    ((r>>7)&0x1)) % 2 == 0)

/* coprocessor operation code for CDP instruction */
#define CPOPC ((Mop->fetch.inst >> 20) & 0x0f)
#define CPEXT ((Mop->fetch.inst >> 5) & 0x07)

/* number of condition flags */
#define MD_MAX_FLAGS            12

/* flag definitions */
#define CF            0x00000001
#define PF            0x00000004
#define AF            0x00000010
#define ZF            0x00000040
#define SF            0x00000080
#define DF            0x00000400
#define OF            0x00000800

#define MKCF(X)            ((!!(X)) << 0)
#define MKPF(X)            ((!!(X)) << 2)
#define MKAF(X)            ((!!(X)) << 4)
#define MKZF(X)            ((!!(X)) << 6)
#define MKSF(X)            ((!!(X)) << 7)
#define MKDF(X)            ((!!(X)) << 10)
#define MKOF(X)            ((!!(X)) << 11)

/* fsw definitions */
#define IE            0x0001
#define DE            0x0002
#define ZE            0x0004
#define OE            0x0008
#define UE            0x0010
#define PE            0x0020
#define FSF           0x0040
#define ES            0x0080
#define C0            0x0100
#define C1            0x0200
#define C2            0x0400
#define C3            0x4000

#define MKIE(X)            ((!!(X)) << 0)
#define MKDE(X)            ((!!(X)) << 1)
#define MKZE(X)            ((!!(X)) << 2)
#define MKOE(X)            ((!!(X)) << 3)
#define MKUE(X)            ((!!(X)) << 4)
#define MKPE(X)            ((!!(X)) << 5)
#define MKFSF(X)            ((!!(X)) << 6)
#define MKES(X)            ((!!(X)) << 7)
#define MKC0(X)            ((!!(X)) << 8)
#define MKC1(X)            ((!!(X)) << 9)
#define MKC2(X)            ((!!(X)) << 10)
#define MKC3(X)            ((!!(X)) << 14)

#define FSW_TOP(X)        (((X) >> 11) & 0x07)
#define SET_FSW_TOP(X,N)    ((X) = (((X) & ~(7<<11)) | (((N) & 7)<<11)))

/* cwd (x87 FPU control word) definitions */
/* exception masks */
#define IM            0x0001
#define DM            0x0002
#define ZM            0x0004
#define OM            0x0008
#define UM            0x0010
#define PM            0x0020

/* helper macros */

/* all undefined flags are 0 */
#define UNDEF        0

/* MUL/IMUL routines */
#define MUL_B(S1,S2)        ((word_t)(S1) * (word_t)(S2))
#define AFLAGS_MULB(R)                            \
  ((((word_t)(R) & 0xff00) != 0)                    \
   ? (MKCF(1)|MKOF(1))                            \
   : (MKCF(0)|MKOF(0)))

#define MULHI_W(S1,S2)                            \
  ((word_t)(((dword_t)(word_t)(S1) * (dword_t)(word_t)(S2)) >> 16))
#define MULHI_D(S1,S2)                            \
  ((dword_t)(((qword_t)(dword_t)(S1) * (qword_t)(dword_t)(S2)) >> 32))

#define MULLO_W(S1,S2)                            \
  ((word_t)((dword_t)(word_t)(S1) * (dword_t)(word_t)(S2)))
#define MULLO_D(S1,S2)                            \
  ((dword_t)((qword_t)(dword_t)(S1) * (qword_t)(dword_t)(S2)))

#define AFLAGS_MULW(RHI)                        \
  (((RHI) != 0) ? (MKOF(1)|MKCF(1)) : (MKOF(0)|MKCF(0)))
#define AFLAGS_MULD(RHI)                        \
  (((RHI) != 0) ? (MKOF(1)|MKCF(1)) : (MKOF(0)|MKCF(0)))

#if 1
#define MULHI_V(S1,S2)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? MULHI_D(S1,S2) : MULHI_W(S1,S2))
#define MULLO_V(S1,S2)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? MULLO_D(S1,S2) : MULLO_W(S1,S2))
#define AFLAGS_MULV(RHI)                        \
  (((RHI) != 0) ? (MKOF(1)|MKCF(1)) : (MKOF(0)|MKCF(0)))
#else
#define MULHI_V(S1,S2)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((dword_t)(((qword_t)(dword_t)(S1) * (qword_t)(dword_t)(S2)) >> 32))    \
   : ((word_t)(((dword_t)(word_t)(S1) * (dword_t)(word_t)(S2)) >> 16)))
#define MULLO_V(S1,S2)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((dword_t)((qword_t)(dword_t)(S1) * (qword_t)(dword_t)(S2)))        \
   : ((word_t)((dword_t)(word_t)(S1) * (dword_t)(word_t)(S2))))
#define AFLAGS_MULV(RHI)                        \
  (((RHI) != 0) ? (MKOF(1)|MKCF(1)) : (MKOF(0)|MKCF(0)))
#endif

#define IMUL_B(S1,S2)                            \
  ((sword_t)(sbyte_t)(S1) * (sword_t)(sbyte_t)(S2))
#define AFLAGS_IMULB(R,S1,S2)                        \
  ((((sword_t)(sbyte_t)(S1)*(sword_t)(sbyte_t)(S2))==(sbyte_t)(R))    \
   ? (MKOF(0)|MKCF(0))                            \
   : (MKOF(1)|MKCF(1)))

#define IMUL_W(S1,S2)        ((sword_t)(S1) * (sword_t)(S2))
#define IMUL_D(S1,S2)        ((sdword_t)(S1) * (sdword_t)(S2))

#define AFLAGS_IMULW(R,S1,S2)                        \
  ((((sdword_t)(sword_t)(S1)*(sdword_t)(sword_t)(S2))==(sword_t)(R))    \
   ? (MKOF(0)|MKCF(0))                            \
   : (MKOF(1)|MKCF(1)))
#define AFLAGS_IMULD(R,S1,S2)                        \
  ((((sqword_t)(sdword_t)(S1)*(sqword_t)(sdword_t)(S2))==(sdword_t)(R))    \
   ? (MKOF(0)|MKCF(0))                            \
   : (MKOF(1)|MKCF(1)))

#if 1
#define IMUL_V(S1,S2)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? IMUL_D(S1,S2) : IMUL_W(S1,S2))
#define AFLAGS_IMULV(R,S1,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_IMULD(R,S1,S2) : AFLAGS_IMULW(R,S1,S2))
#else
#define IMUL_V(S1,S2)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((sdword_t)(S1) * (sdword_t)(S2))                    \
   : ((sword_t)(S1) * (sword_t)(S2)))
#define AFLAGS_IMULV(R,S1,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((((sqword_t)(sdword_t)(S1)*(sqword_t)(sdword_t)(S2))==(sdword_t)(R))    \
     ? (MKOF(0)|MKCF(0))                        \
     : (MKOF(1)|MKCF(1)))                        \
   : ((((sdword_t)(sword_t)(S1)*(sdword_t)(sword_t)(S2))==(sword_t)(R))    \
     ? (MKOF(0)|MKCF(0))                        \
     : (MKOF(1)|MKCF(1))))
#endif

#define IMULHI_W(S1,S2)                            \
  ((word_t)(((sdword_t)(sword_t)(S1) * (sdword_t)(sword_t)(S2)) >> 16))
#define IMULHI_D(S1,S2)                            \
  ((dword_t)(((sqword_t)(sdword_t)(S1) * (sqword_t)(sdword_t)(S2)) >> 32))

#define IMULLO_W(S1,S2)                            \
  ((word_t)((sdword_t)(sword_t)(S1) * (sdword_t)(sword_t)(S2)))
#define IMULLO_D(S1,S2)                            \
  ((dword_t)((sqword_t)(sdword_t)(S1) * (sqword_t)(sdword_t)(S2)))

#if 1
#define IMULHI_V(S1,S2)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? IMULHI_D(S1,S2) : IMULHI_W(S1,S2))
#define IMULLO_V(S1,S2)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? IMULLO_D(S1,S2) : IMULLO_W(S1,S2))

#define AFLAGS_IMULHIV(RHI,RLO)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? AFLAGS_IMULHID(RHI,RLO)                        \
   : AFLAGS_IMULHIW(RHI,RLO))

#define AFLAGS_IMULHIW(RHI,RLO)                        \
  (((((RHI) == 0xffff) && ((RLO) & 0x8000))                \
    || (((RHI) == 0) && !((RLO) & 0x8000)))                \
   ? (MKOF(0)|MKCF(0))                            \
   : (MKOF(1)|MKCF(1)))
#define AFLAGS_IMULHID(RHI,RLO)                        \
  (((((RHI) == 0xffffffff) && ((RLO) & 0x80000000))            \
    || (((RHI) == 0) && !((RLO) & 0x80000000)))                \
   ? (MKOF(0)|MKCF(0))                            \
   : (MKOF(1)|MKCF(1)))
#else
#define IMULHI_V(S1,S2)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((dword_t)(((sqword_t)(sdword_t)(S1) * (sqword_t)(sdword_t)(S2)) >> 32))\
   : ((word_t)(((sdword_t)(sword_t)(S1) * (sdword_t)(sword_t)(S2)) >> 16)))
#define IMULLO_V(S1,S2)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((dword_t)((sqword_t)(sdword_t)(S1) * (sqword_t)(sdword_t)(S2)))    \
   : ((word_t)((sdword_t)(sword_t)(S1) * (sdword_t)(sword_t)(S2))))
/* BUG? */
#define AFLAGS_IMULHIV(RHI,RLO)                        \
  (((((RHI) == 0xffffffff) && ((RLO) & 0x80000000))            \
    || (((RHI) == 0) && !((RLO) & 0x80000000)))                \
   ? (MKOF(0)|MKCF(0))                            \
   : (MKOF(1)|MKCF(1)))
#endif

/* DIV/IDIV routines */
#define QUO_B(S1,S2)        ((byte_t)((word_t)(S1) / (byte_t)(S2)))
#define REM_B(S1,S2)        ((byte_t)((word_t)(S1) % (byte_t)(S2)))
#define AFLAGS_DIVB()        (UNDEF)

#define QUO_W(S1H,S1L,S2)                        \
  ((word_t)(((dword_t)(((dword_t)((S1H) & 0xffff))<<16)            \
      | ((dword_t)((S1L) & 0xffff))) / ((dword_t)((S2) & 0xffff))))
#define QUO_D(S1H,S1L,S2)                        \
  ((dword_t)(((qword_t)(((qword_t)(S1H))<<32)                \
      | ((qword_t)(S1L))) / ((qword_t)(S2))))

#define REM_W(S1H,S1L,S2)                        \
  ((word_t)(((dword_t)(((dword_t)((S1H) & 0xffff))<<16)            \
      | ((dword_t)((S1L) & 0xffff))) % ((dword_t)((S2) & 0xffff))))
#define REM_D(S1H,S1L,S2)                        \
  ((dword_t)(((qword_t)(((qword_t)(S1H))<<32)                \
      | ((qword_t)(S1L))) % ((qword_t)(S2))))

#define AFLAGS_DIVW()        (UNDEF)
#define AFLAGS_DIVD()        (UNDEF)

#if 1
#define QUO_V(S1H,S1L,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? QUO_D(S1H,S1L,S2) : QUO_W(S1H,S1L,S2))
#define REM_V(S1H,S1L,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? REM_D(S1H,S1L,S2) : REM_W(S1H,S1L,S2))
#define AFLAGS_DIVV()        (UNDEF)
#else
#define QUO_V(S1H,S1L,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((dword_t)(((qword_t)(((qword_t)(S1H))<<32)                \
         | ((qword_t)(S1L))) / ((qword_t)(S2))))            \
   : ((word_t)(((dword_t)(((dword_t)((S1H) & 0xffff))<<16)        \
         | ((dword_t)((S1L) & 0xffff))) / ((dword_t)((S2) & 0xffff)))))
#define REM_V(S1H,S1L,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((dword_t)(((qword_t)(((qword_t)(S1H))<<32)                \
         | ((qword_t)(S1L))) % ((qword_t)(S2))))            \
   : ((word_t)(((dword_t)(((dword_t)((S1H) & 0xffff))<<16)        \
         | ((dword_t)((S1L) & 0xffff))) % ((dword_t)((S2) & 0xffff)))))
#define AFLAGS_DIVV()        (UNDEF)
#endif

#define IQUO_B(S1,S2)        ((sbyte_t)((sword_t)(S1) / (sbyte_t)(S2)))
#define IREM_B(S1,S2)        ((sbyte_t)((sword_t)(S1) % (sbyte_t)(S2)))
#define AFLAGS_IDIVB()        (UNDEF)

#define IQUO_W(S1H,S1L,S2)                        \
  ((word_t)(((sdword_t)((((sdword_t)((S1H) & 0xffff))<<16)            \
        | ((sdword_t)((S1L) & 0xffff)))) / ((sdword_t)((S2) & 0xffff))))
#define IQUO_D(S1H,S1L,S2)                        \
  ((dword_t)(((sqword_t)((((sqword_t)(S1H))<<32)                \
        | ((sqword_t)(S1L)))) / ((sqword_t)(S2))))

#define IREM_W(S1H,S1L,S2)                        \
  ((word_t)(((sdword_t)((((sdword_t)((S1H) & 0xffff))<<16)            \
        | ((sdword_t)((S1L) & 0xffff)))) % ((sdword_t)((S2) & 0xffff))))
#define IREM_D(S1H,S1L,S2)                        \
  ((dword_t)(((sqword_t)((((sqword_t)(S1H))<<32)                \
        | ((sqword_t)(S1L)))) % ((sqword_t)(S2))))

#define AFLAGS_IDIVW()        (UNDEF)
#define AFLAGS_IDIVD()        (UNDEF)

#if 1
#define IQUO_V(S1H,S1L,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? IQUO_D(S1H,S1L,S2) : IQUO_W(S1H,S1L,S2))
#define IREM_V(S1H,S1L,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? IREM_D(S1H,S1L,S2) : IREM_W(S1H,S1L,S2))
#define AFLAGS_IDIVV()        (UNDEF)
#else
#define IQUO_V(S1H,S1L,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((dword_t)(((sqword_t)((((qword_t)(S1H))<<32)            \
           | ((qword_t)(S1L)))) / ((sqword_t)(qword_t)(S2))))    \
   : ((word_t)(((sdword_t)((((dword_t)((S1H) & 0xffff))<<16)        \
           | ((dword_t)((S1L) & 0xffff)))) / ((sdword_t)(dword_t)((S2) & 0xffff)))))
#define IREM_V(S1H,S1L,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((dword_t)(((sqword_t)((((qword_t)(S1H))<<32)            \
           | ((qword_t)(S1L)))) % ((sqword_t)(qword_t)(S2))))    \
   : ((word_t)(((sdword_t)((((dword_t)((S1H) & 0xffff))<<16)        \
           | ((dword_t)((S1L) & 0xffff)))) % ((sdword_t)(dword_t)((S2) & 0xffff)))))
#define AFLAGS_IDIVV()        (UNDEF)
#endif
/* compute ADD flags */
#define AFLAGS_ADDB(R,S1,S2)                        \
  (MKOF((((S1) & 0x80) == ((S2) & 0x80))                \
        && (((R) & 0x80) != ((S2) & 0x80)))                \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF((byte_t)((R) & 0x0f) < (byte_t)((S1) & 0x0f))            \
   | MKCF((byte_t)(R) < (byte_t)(S1))                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_ADDW(R,S1,S2)                        \
  (MKOF((((S1) & 0x8000) == ((S2) & 0x8000))                \
        && (((R) & 0x8000) != ((S2) & 0x8000)))                \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF((word_t)((R) & 0x0f) < (word_t)((S1) & 0x0f))            \
   | MKCF((word_t)(R) < (word_t)(S1))                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_ADDD(R,S1,S2)                        \
  (MKOF((((S1) & 0x80000000) == ((S2) & 0x80000000))            \
        && (((R) & 0x80000000) != ((S2) & 0x80000000)))            \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF((dword_t)((R) & 0x0f) < (dword_t)((S1) & 0x0f))            \
   | MKCF((dword_t)(R) < (dword_t)(S1))                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_ADDV(R,S1,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_ADDD(R, S1, S2) : AFLAGS_ADDW(R, S1, S2))

/* compute SUB flags */
#define AFLAGS_SUBB(R,S1,S2)                        \
  (MKOF((((S1) & 0x80) != ((S2) & 0x80))                \
        && (((R) & 0x80) != ((S1) & 0x80)))                \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF((byte_t)((S1) & 0x0f) < (byte_t)((S2) & 0x0f))        \
   | MKCF((byte_t)(S1) < (byte_t)(S2))                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_SUBW(R,S1,S2)                        \
  (MKOF((((S1) & 0x8000) != ((S2) & 0x8000))                \
        && (((R) & 0x8000) != ((S1) & 0x8000)))                \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF((word_t)((S1) & 0x0f) < (word_t)((S2) & 0x0f))        \
   | MKCF((word_t)(S1) < (word_t)(S2))                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_SUBD(R,S1,S2)                        \
  (MKOF((((S1) & 0x80000000) != ((S2) & 0x80000000))            \
        && (((R) & 0x80000000) != ((S1) & 0x80000000)))            \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF((dword_t)((S1) & 0x0f) < (dword_t)((S2) & 0x0f))        \
   | MKCF((dword_t)(S1) < (dword_t)(S2))                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_SUBV(R,S1,S2)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_SUBD(R, S1, S2) : AFLAGS_SUBW(R, S1, S2))

/* compute ALU operation flags */
#define AFLAGS_ALUB(R)                            \
  (MKOF(0)                                \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(0)                                \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_ALUW(R)                            \
  (MKOF(0)                                \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(0)                                \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_ALUD(R)                            \
  (MKOF(0)                                \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(0)                                \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_ALUV(R)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_ALUD(R) : AFLAGS_ALUW(R))

/* compute INC flags */
#define AFLAGS_INCB(R)                            \
  (MKOF((byte_t)(R) == 0x80)                        \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF(((R) & 0x0f) == 0)                        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_INCW(R)                            \
  (MKOF((word_t)(R) == 0x8000)                        \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF(((R) & 0x0f) == 0)                        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_INCD(R)                            \
  (MKOF((dword_t)(R) == 0x80000000)                    \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF(((R) & 0x0f) == 0)                        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_INCV(R)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_INCD(R) : AFLAGS_INCW(R))

/* compute DEC flags */
#define AFLAGS_DECB(R)                            \
  (MKOF((byte_t)(R) == 0x7f)                        \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF(((R) & 0x0f) == 0x0f)                        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_DECW(R)                            \
  (MKOF((word_t)(R) == 0x7fff)                        \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF(((R) & 0x0f) == 0x0f)                        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_DECD(R)                            \
  (MKOF((dword_t)(R) == 0x7fffffff)                    \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF(((R) & 0x0f) == 0x0f)                        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_DECV(R)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_DECD(R) : AFLAGS_DECW(R))

/* compute ADD flags */
#define AFLAGS_ADCB(R,S1,S2,C)                        \
  (MKOF((((S1) & 0x80) == ((S2) & 0x80))                \
        && (((R) & 0x80) != ((S2) & 0x80)))                \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF((((S1) ^ (S2)) ^ (R)) & 0x10)                    \
   | MKCF(((byte_t)(R) < (byte_t)(S1))                    \
     || ((C) && ((byte_t)(R) == (byte_t)(S1))))            \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_ADCW(R,S1,S2,C)                        \
  (MKOF((((S1) & 0x8000) == ((S2) & 0x8000))                \
        && (((R) & 0x8000) != ((S2) & 0x8000)))                \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF((((S1) ^ (S2)) ^ (R)) & 0x10)                    \
   | MKCF(((word_t)(R) < (word_t)(S1))                    \
     || ((C) && ((word_t)(R) == (word_t)(S1))))            \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_ADCD(R,S1,S2,C)                        \
  (MKOF((((S1) & 0x80000000) == ((S2) & 0x80000000))            \
        && (((R) & 0x80000000) != ((S2) & 0x80000000)))            \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF((((S1) ^ (S2)) ^ (R)) & 0x10)                    \
   | MKCF(((dword_t)(R) < (dword_t)(S1))                    \
     || ((C) && ((dword_t)(R) == (dword_t)(S1))))            \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_ADCV(R,S1,S2,C)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? AFLAGS_ADCD(R, S1, S2, C)                        \
   : AFLAGS_ADCW(R, S1, S2, C))

/* compute SBB flags */
#define AFLAGS_SBBB(R,S1,S2,C)                        \
  (MKOF((((S1) & 0x80) != ((S2) & 0x80))                \
        && (((R) & 0x80) != ((S1) & 0x80)))                \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF((((S1) ^ (S2)) ^ (R)) & 0x10)                    \
   | MKCF(((byte_t)(S1) < (byte_t)(R))                    \
     || ((C) && ((byte_t)(S2) == 0xff)))                \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_SBBW(R,S1,S2,C)                        \
  (MKOF((((S1) & 0x8000) != ((S2) & 0x8000))                \
        && (((R) & 0x8000) != ((S1) & 0x8000)))                \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF((((S1) ^ (S2)) ^ (R)) & 0x10)                    \
   | MKCF(((word_t)(S1) < (word_t)(R))                    \
     || ((C) && ((word_t)(S2) == 0xffff)))                \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_SBBD(R,S1,S2,C)                        \
  (MKOF((((S1) & 0x80000000) != ((S2) & 0x80000000))            \
        && (((R) & 0x80000000) != ((S1) & 0x80000000)))            \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF((((S1) ^ (S2)) ^ (R)) & 0x10)                    \
   | MKCF(((dword_t)(S1) < (dword_t)(R))                    \
     || ((C) && ((dword_t)(S2) == 0xffffffff)))            \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_SBBV(R,S1,S2,C)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? AFLAGS_SBBD(R, S1, S2, C)                        \
   : AFLAGS_SBBW(R, S1, S2, C))

/* negation flags */
#define AFLAGS_NEGB(R,S)                        \
  (MKOF((byte_t)(S) == 0x80)                        \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF(((S) & 0x0f) > 0)                        \
   | MKCF((byte_t)(S) != 0)                        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_NEGW(R,S)                        \
  (MKOF((word_t)(S) == 0x8000)                        \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF(((S) & 0x0f) > 0)                        \
   | MKCF((word_t)(S) != 0)                        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_NEGD(R,S)                        \
  (MKOF((dword_t)(S) == 0x80000000)                    \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF(((S) & 0x0f) > 0)                        \
   | MKCF((dword_t)(S) != 0)                        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_NEGV(R,S)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_NEGD(R,S) : AFLAGS_NEGW(R,S))

/* rotates */
#define ROL_B(X,N)                            \
  (((X) << (N)) | ((X) >> (8-(N))))
#define AFLAGS_ROLB(R,S,N)                        \
  (MKCF((R) & 0x01)                            \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80) : UNDEF))
#define ROL_W(X,N)                            \
  (((X) << (N)) | ((X) >> (16-(N))))
#define AFLAGS_ROLW(R,S,N)                        \
  (MKCF((R) & 0x01)                            \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x8000) : UNDEF))
#define ROL_D(X,N)                            \
  (((X) << (N)) | ((X) >> (32-(N))))
#define AFLAGS_ROLD(R,S,N)                        \
  (MKCF((R) & 0x01)                            \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80000000) : UNDEF))
#define ROL_V(X,N)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? ROL_D(X,N) : ROL_W(X,N))
#define AFLAGS_ROLV(R,S,N)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_ROLD(R,S,N) : AFLAGS_ROLW(R,S,N))

#define ROR_B(X,N)                            \
  (((X) >> (N)) | ((X) << (8-(N))))
#define AFLAGS_RORB(R,S,N)                        \
  (MKCF((R) & 0x80)                            \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80) : UNDEF))
#define ROR_W(X,N)                            \
  (((X) >> (N)) | ((X) << (16-(N))))
#define AFLAGS_RORW(R,S,N)                        \
  (MKCF((R) & 0x8000)                            \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x8000) : UNDEF))
#define ROR_D(X,N)                            \
  (((X) >> (N)) | ((X) << (32-(N))))
#define AFLAGS_RORD(R,S,N)                        \
  (MKCF((R) & 0x80000000)                        \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80000000) : UNDEF))
#define ROR_V(X,N)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? ROR_D(X,N) : ROR_W(X,N))
#define AFLAGS_RORV(R,S,N)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_RORD(R,S,N) : AFLAGS_RORW(R,S,N))

#define RCL_B(X,CF,N)                            \
  ({ byte_t _res, n = (N);                        \
   if ((N) != 0)                            \
   _res = ((n == 1)                            \
     ? (((X) << 1) | (CF))                        \
     : (((X) << n) | ((CF) << (n - 1)) | ((X) >> (9 - n))));        \
   else                                \
   _res = (X);                            \
   _res;                                \
   }) 
#define AFLAGS_RCLB(R,S,N)                        \
  (MKCF(((S) >> (8 - (N))) & 0x01)                    \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80) : UNDEF))
#define RCL_W(X,CF,N)                            \
  ({ word_t _res, n = (N);                        \
   if ((N) != 0)                            \
   _res = ((n == 1)                            \
     ? (((X) << 1) | (CF))                        \
     : (((X) << n) | ((CF) << (n - 1)) | ((X) >> (17 - n))));    \
   else                                \
   _res = (X);                            \
   _res;                                \
   })
#define AFLAGS_RCLW(R,S,N)                        \
  (MKCF(((S) >> (16 - (N))) & 0x01)                    \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x8000) : UNDEF))
#define RCL_D(X,CF,N)                            \
  ({ dword_t _res, n = (N);                        \
   if ((N) != 0)                            \
   _res = ((n == 1)                            \
     ? (((X) << 1) | (CF))                        \
     : (((X) << n) | ((CF) << (n - 1)) | ((X) >> (33 - n))));    \
   else                                \
   _res = (X);                            \
   _res;                                \
   })
#define AFLAGS_RCLD(R,S,N)                        \
  (MKCF(((S) >> (32 - (N))) & 0x01)                    \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80000000) : UNDEF))
#define RCL_V(X,CF,N)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? RCL_D(X,CF,N) : RCL_W(X,CF,N))
#define AFLAGS_RCLV(R,S,N)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_RCLD(R,S,N) : AFLAGS_RCLW(R,S,N))

#define RCR_B(X,CF,N)                            \
  ({ byte_t _res, n = (N);                        \
   if ((N) != 0)                            \
   _res = ((n == 1)                            \
     ? (((X) >> 1) | ((CF) << 7))                    \
     : (((X) >> n) | ((CF) << (8 - n)) | ((X) << (9 - n))));        \
   else                                \
   _res = (X);                            \
   _res;                                \
   })
#define AFLAGS_RCRB(R,S,N)                        \
  (MKCF(((S) >> ((N) - 1)) & 0x01)                    \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80) : UNDEF))
#define RCR_W(X,CF,N)                            \
  ({ word_t _res, n = (N);                        \
   if ((N) != 0)                            \
   _res = ((n == 1)                            \
     ? (((X) >> 1) | ((CF) << 15))                    \
     : (((X) >> n) | ((CF) << (16 - n)) | ((X) << (17 - n))));    \
   else                                \
   _res = (X);                            \
   _res;                                \
   })
#define AFLAGS_RCRW(R,S,N)                        \
  (MKCF(((S) >> ((N) - 1)) & 0x01)                    \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x8000) : UNDEF))
#define RCR_D(X,CF,N)                            \
  ({ dword_t _res, n = (N);                        \
   if ((N) != 0)                            \
   _res = ((n == 1)                            \
     ? (((X) >> 1) | ((CF) << 31))                    \
     : (((X) >> n) | ((CF) << (32 - n)) | ((X) << (33 - n))));    \
   else                                \
   _res = (X);                            \
   _res;                                \
   })
#define AFLAGS_RCRD(R,S,N)                        \
  (MKCF(((S) >> ((N) - 1)) & 0x01)                    \
   | MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80000000) : UNDEF))
#define RCR_V(X,CF,N)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? RCR_D(X,CF,N) : RCR_W(X,CF,N))
#define AFLAGS_RCRV(R,S,N)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_RCRD(R,S,N) : AFLAGS_RCRW(R,S,N))

#define SHL_B(X,N)                            \
  ((X) << (N))
#define AFLAGS_SHLB(R,S,N)                        \
  (MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80) : UNDEF)            \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((N) <= 8) ? (((S) >> (8 - (N))) & 0x01) : 0)            \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define SHL_W(X,N)                            \
  ((X) << (N))
#define AFLAGS_SHLW(R,S,N)                        \
  (MKOF(((N) == 1) ? (((S) ^ (R)) & 0x8000) : UNDEF)            \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((N) <= 16) ? (((S) >> (16 - (N))) & 0x01) : 0)        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define SHL_D(X,N)                            \
  ((X) << (N))
#define AFLAGS_SHLD(R,S,N)                        \
  (MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80000000) : UNDEF)        \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((N) <= 32) ? (((S) >> (32 - (N))) & 0x01) : 0)        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define SHL_V(X,N)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? SHL_D(X,N) : SHL_W(X,N))
#define AFLAGS_SHLV(R,S,N)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_SHLD(R,S,N) : AFLAGS_SHLW(R,S,N))

#define SHR_B(X,N)                            \
  ((X) >> (N))
#define AFLAGS_SHRB(R,S,N)                        \
  (MKOF(((N) == 1) ? ((S) & 0x80) : UNDEF)                \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((S) >> ((N) - 1)) & 0x01)                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define SHR_W(X,N)                            \
  ((X) >> (N))
#define AFLAGS_SHRW(R,S,N)                        \
  (MKOF(((N) == 1) ? ((S) & 0x8000) : UNDEF)                \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((S) >> ((N) - 1)) & 0x01)                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define SHR_D(X,N)                            \
  ((X) >> (N))
#define AFLAGS_SHRD(R,S,N)                        \
  (MKOF(((N) == 1) ? ((S) & 0x80000000) : UNDEF)            \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((S) >> ((N) - 1)) & 0x01)                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define SHR_V(X,N)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? SHR_D(X,N) : SHR_W(X,N))
#define AFLAGS_SHRV(R,S,N)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_SHRD(R,S,N) : AFLAGS_SHRW(R,S,N))

#define SAR_B(X,N)                            \
  (((N) < 8)                                \
   ? (((X) & 0x80) ? (((X) >> (N)) | (0xff << (8-(N)))) : ((X) >> (N)))    \
   : (((X) & 0x80) ? 0xff : 0))
#define AFLAGS_SARB(R,S,N)                        \
  (MKOF(((N) == 1) ? 0 : UNDEF)                        \
   | MKSF((byte_t)(R) >= 0x80)                        \
   | MKZF((byte_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((N) < 8) ? (((S) >> ((N)-1)) & 0x01) : ((S) & 0x80))        \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define SAR_W(X,N)                            \
  (((N) < 16)                                \
   ? (((X) & 0x8000) ? (((X) >> (N)) | (0xffff << (16-(N)))) : ((X) >> (N)))\
   : (((X) & 0x8000) ? 0xffff : 0))
#define AFLAGS_SARW(R,S,N)                        \
  (MKOF(((N) == 1) ? 0 : UNDEF)                        \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((N) < 16) ? (((S) >> ((N)-1)) & 0x01) : ((S) & 0x8000))    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define SAR_D(X,N)                            \
  (((N) < 32)                                \
   ? (((N) == 0)                            \
     ? (X)                                \
     : (((X) & 0x80000000)                        \
       ? (((X) >> (N)) | (0xffffffff << (32-(N)))) : ((X) >> (N))))    \
   : (((X) & 0x80000000) ? 0xffffffff : 0))
#define AFLAGS_SARD(R,S,N)                        \
  (MKOF(((N) == 1) ? 0 : UNDEF)                        \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((N) < 32) ? (((S) >> ((N)-1)) & 0x01) : ((S) & 0x80000000))    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define SAR_V(X,N)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? SAR_D(X,N) : SAR_W(X,N))
#define AFLAGS_SARV(R,S,N)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_SARD(R,S,N) : AFLAGS_SARW(R,S,N))

#define AFLAGS_SHLDD(R,S,N)                        \
  (MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80000000) : UNDEF)        \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((S) >> (32 - (N))) & 1)                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_SHLDW(R,S,N)                        \
  (MKOF(((N) == 1) ? (((S) ^ (R)) & 0x8000) : UNDEF)            \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((S) >> (16 - (N))) & 1)                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_SHLDV(R,S,N)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_SHLDD(R,S,N) : AFLAGS_SHLDW(R,S,N))

#define AFLAGS_SHRDD(R,S,N)                        \
  (MKOF(((N) == 1) ? (((S) ^ (R)) & 0x80000000) : UNDEF)        \
   | MKSF((dword_t)(R) >= 0x80000000)                    \
   | MKZF((dword_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((S) >> ((N) - 1)) & 1)                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_SHRDW(R,S,N)                        \
  (MKOF(((N) == 1) ? (((S) ^ (R)) & 0x8000) : UNDEF)            \
   | MKSF((word_t)(R) >= 0x8000)                    \
   | MKZF((word_t)(R) == 0)                        \
   | MKAF(UNDEF)                            \
   | MKCF(((S) >> ((N) - 1)) & 1)                    \
   | MKPF(IS_EVEN_ONES((byte_t)(R))) )
#define AFLAGS_SHRDV(R,S,N)                        \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_SHRDD(R,S,N) : AFLAGS_SHRDW(R,S,N))

/* byte swap */
#define BSWAP(X)                            \
  ((((X) >> 24) & 0x000000ff)                        \
   | (((X) >> 8) & 0x0000ff00)                        \
   | (((X) << 8) & 0x00ff0000)                        \
   | (((X) << 24) & 0xff000000))

/* bit twiddlers */
/* FIXME: should undef all other flags */

#define BTS_V(X,N)        ((X) | (1 << (N)))
#define BTR_V(X,N)        ((X) & ~((dword_t)1 << (N)))
#define BTC_V(X,N)        ((X) ^ (1 << (N)))
#define AFLAGS_BTV(X,N)        MKCF(((X) >> (N)) & 0x01)

/* movsx/movzs primitives */
#define MOVZX_WB(S)        ((word_t)(byte_t)(S))
#define MOVZX_DB(S)        ((dword_t)(byte_t)(S))
#define MOVZX_WW(S)        ((word_t)(S))
#define MOVZX_DW(S)        ((dword_t)(word_t)(S))
#if 1
#define MOVZX_VB(S)    ((Mop->fetch.inst.mode & MODE_OPER32) ? MOVZX_DB(S) : MOVZX_WB(S))
#define MOVZX_VW(S)    ((Mop->fetch.inst.mode & MODE_OPER32) ? MOVZX_DW(S) : MOVZX_WW(S))
#else
#define MOVZX_VB(S)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((dword_t)(byte_t)(S))                        \
   : ((word_t)(byte_t)(S)))
#define MOVZX_VW(S)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((dword_t)(word_t)(S))                        \
   : ((word_t)(S)))
#endif

#define MOVSX_WB(S)        ((sword_t)(sbyte_t)(S))
#define MOVSX_DB(S)        ((sdword_t)(sbyte_t)(S))
#define MOVSX_WW(S)        ((sword_t)(S))
#define MOVSX_DW(S)        ((sdword_t)(sword_t)(S))
#if 1
#define MOVSX_VB(S)    ((Mop->fetch.inst.mode & MODE_OPER32) ? MOVSX_DB(S) : MOVSX_WB(S))
#define MOVSX_VW(S)    ((Mop->fetch.inst.mode & MODE_OPER32) ? MOVSX_DW(S) : MOVSX_WW(S))
#else
#define MOVSX_VB(S)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((sdword_t)(sbyte_t)(S))                        \
   : ((sword_t)(sbyte_t)(S)))
#define MOVSX_VW(S)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? ((sdword_t)(sword_t)(S))                        \
   : ((sword_t)(S)))
#endif

/* bit scan functions */
#define BSF_W(S)    BSF_V(S)
#define BSF_D(S)    BSF_V(S)
#define BSF_V(S)                            \
  ({                                    \
   dword_t idx = 0;                            \
   \
   if ((S) != 0)                            \
   {                                \
   while (((S) & (1 << idx)) == 0)                \
   idx++;                            \
   }                                \
   idx;                                \
   })
/* FIXME: should undef all other flags */
#define AFLAGS_BSFW(S)        MKZF((word_t)(S) == 0)
#define AFLAGS_BSFD(S)        MKZF((dword_t)(S) == 0)
#define AFLAGS_BSFV(S)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_BSFD(S) : AFLAGS_BSFW(S))

#define BSR_W(S)                            \
  ({                                    \
   dword_t idx = BITSIZE_W - 1;                    \
   \
   if ((S) != 0)                            \
   {                                \
   while (((S) & (1 << idx)) == 0)                \
   idx--;                            \
   }                                \
   idx;                                \
   })
#define BSR_D(S)                            \
  ({                                    \
   dword_t idx = BITSIZE_D - 1;                    \
   \
   if ((S) != 0)                            \
   {                                \
   while (((S) & (1 << idx)) == 0)                \
   idx--;                            \
   }                                \
   idx;                                \
   })
#define BSR_V(S)                            \
  ({                                    \
   dword_t idx = BITSIZE_V - 1;                    \
   \
   if ((S) != 0)                            \
   {                                \
   while (((S) & (1 << idx)) == 0)                \
   idx--;                            \
   }                                \
   idx;                                \
   })
/* FIXME: should undef all other flags */
#define AFLAGS_BSRW(S)        MKZF((word_t)(S) == 0)
#define AFLAGS_BSRD(S)        MKZF((dword_t)(S) == 0)
#define AFLAGS_BSRV(S)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32) ? AFLAGS_BSRD(S) : AFLAGS_BSRW(S))

/* FCOM flag computation */
#define FSW_FCOM(S1,S2)                            \
  (((S1) > (S2))                            \
   ? (MKC3(0)|MKC2(0)|MKC0(0))                        \
   : (((S1) < (S2))                            \
     ? (MKC3(0)|MKC2(0)|MKC0(1))                    \
     : (((S1) == (S2))                            \
       ? (MKC3(1)|MKC2(0)|MKC0(0))                    \
       : (MKC3(1)|MKC2(1)|MKC0(1)))))

/* FUCOM flag computation */
#define FSW_FUCOM(S1,S2)                        \
  (((S1) > (S2))                            \
   ? (MKC3(0)|MKC2(0)|MKC0(0))                        \
   : (((S1) < (S2))                            \
     ? (MKC3(0)|MKC2(0)|MKC0(1))                    \
     : (((S1) == (S2))                            \
       ? (MKC3(1)|MKC2(0)|MKC0(0))                    \
       : (MKC3(1)|MKC2(1)|MKC0(1)))))

/* FCOMI flag computation */
#define AFLAGS_FCOMI(S1,S2)                        \
  (((S1) > (S2))                            \
   ? (MKZF(0)|MKPF(0)|MKCF(0))                        \
   : (((S1) < (S2))                            \
     ? (MKZF(0)|MKPF(0)|MKZF(1))                    \
     : (((S1) == (S2))                            \
       ? (MKZF(1)|MKPF(0)|MKCF(0))                    \
       : (MKZF(1)|MKPF(1)|MKCF(1)))))

/* FUCOMI flag computation */
#define AFLAGS_FUCOMI(S1,S2)                        \
  (((S1) > (S2))                            \
   ? (MKZF(0)|MKPF(0)|MKCF(0))                        \
   : (((S1) < (S2))                            \
     ? (MKZF(0)|MKPF(0)|MKZF(1))                    \
     : (((S1) == (S2))                            \
       ? (MKZF(1)|MKPF(0)|MKCF(0))                    \
       : (MKZF(1)|MKPF(1)|MKCF(1)))))

/* FP constants */
#define FPCONST(LC)                            \
  ({                                    \
   efloat_t _res;                            \
   \
   switch (LC)                            \
   {                                \
   case 0:    _res = 1.0; break;                    \
   case 1:    _res = log(10.0)/log(2.0); break;            \
   case 2:    _res = log(M_E)/log(2.0); break;            \
   case 3:    _res = M_PI; break;                    \
   case 4:    _res = log(2.0)/log(10.0); break;            \
   case 5:    _res = log(2.0)/log(M_E); break;            \
   case 6:    _res = 0.0; break;                    \
   default:    panic("bogus FPCONST");                    \
   }                                \
   _res;                                \
   })

/* branch conditions */
#define CC_O        0
#define CC_NO        1
#define CC_B        2
#define CC_NB        3
#define CC_E        4
#define CC_NE        5
#define CC_BE        6
#define CC_NBE        7
#define CC_S        8
#define CC_NS        9
#define CC_P        10
#define CC_NP        11
#define CC_L        12
#define CC_NL        13
#define CC_LE        14
#define CC_NLE        15

/* flags evaluator */
int md_cc_eval(const int cond, const dword_t aflags, bool * bogus);
#define CC_EVAL(CC,F)        md_cc_eval(CC, F, bogus)

/* FP branch conditions */
#define FCC_B        0
#define FCC_E        1
#define FCC_BE        2
#define FCC_U        3
#define FCC_NB        4
#define FCC_NE        5
#define FCC_NBE        6
#define FCC_NU        7

/* floating point flags evaluator */
int md_fcc_eval(int cond, dword_t aflags, bool * bogus);
#define FCC_EVAL(FCC,F)        md_fcc_eval(FCC, F, bogus)

/* mode-specific constants */
#define SIZE_V            ((Mop->fetch.inst.mode & MODE_OPER32) ? 4 : 2)
#define BITSIZE_V        ((Mop->fetch.inst.mode & MODE_OPER32) ? 32 : 16)
#define LOGSIZE_V        ((Mop->fetch.inst.mode & MODE_OPER32) ? 2 : 1)
#define LOGBITSIZE_V        ((Mop->fetch.inst.mode & MODE_OPER32) ? 5 : 4)

/* variable sign extension */
#define SIGN_V(X)                            \
  ((Mop->fetch.inst.mode & MODE_OPER32)                        \
   ? (((X) & 0x80000000) ? 0xffffffff : 0x00000000)            \
   : (((X) & 0x8000) ? 0xffff : 0x0000))

#define QW2E(S1,S2)                            \
  ({ union { efloat_t e; struct { qword_t lo; dword_t hi; } d; } x;    \
   x.d.lo = S1;                            \
   x.d.hi = S2;                            \
   x.e;                                \
   })

#define E2Qw(E)                                \
  ({ union { efloat_t e; struct { qword_t lo; dword_t hi; } d; } x;    \
   x.e = E;                                \
   x.d.lo;                                \
   })

#define E2qW(E)                                \
  ({ union { efloat_t e; struct { qword_t lo; dword_t hi; } d; } x;    \
   x.e = E;                                \
   x.d.hi;                                \
   })


/* default target PC handling */
#ifndef SET_TPC
#define SET_TPC(PC)    (PC)
#define SET_TPC_D(PC)  (PC)
#define SET_TPC_V(PC)  (PC)
#endif /* SET_TPC */

/*
 * various other helper macros/functions
 */

/* non-zero if system call is an exit() */
#define OSF_SYS_exit            1
#define MD_EXIT_SYSCALL(INST,REGS)                        \
  ((REGS)->regs_R.dw[MD_REG_EAX] == OSF_SYS_exit)

/* non-zero if system call is a write to stdout/stderr */
#define OSF_SYS_write            4
#define MD_OUTPUT_SYSCALL(INST,REGS)                        \
  ((REGS)->regs_R.dw[MD_REG_EAX] == OSF_SYS_write            \
   && ((REGS)->regs_R.dw[MD_REG_EBX] == /* stdout */1            \
     || (REGS)->regs_R.dw[MD_REG_EBX] == /* stderr */2))

/* returns stream of an output system call, translated to host */
#define MD_STREAM_FILENO(REGS)        ((REGS)->regs_R.dw[MD_REG_EBX])

/* XXX returns non-zero if instruction is a function call */
#define MD_IS_CALL(OPFLAGS)          ((OPFLAGS)&F_CALL)

/* returns non-zero if instruction is a function return */
#define MD_IS_RETURN(OPFLAGS)        ((OPFLAGS)&F_RETN)

/* returns non-zero if instruction is an indirect jump */
#define MD_IS_INDIR(OPFLAGS)         ((OPFLAGS)&F_INDIRJMP)

/* globbing/fusion masks */
#define FUSION_NONE 0x0000LL
#define FUSION_LOAD_OP 0x0001LL
#define FUSION_STA_STD 0x0002LL
#define FUSION_PARTIAL 0x0004LL  /* for partial-register-write merging uops */
/* to add: OP_OP, OP_ST */

#define FUSION_TYPE(uop) (((uop_inst_t)((uop)->decode.raw_op)) >> 32)


/*
 * EIO package configuration/macros
 */

/* expected EIO file format */
#define MD_EIO_FILE_FORMAT  EIO_X86_FORMAT

#define MD_MISC_REGS_TO_EXO(REGS)                    \
  exo_new(ec_list,                            \
      /*icnt*/exo_new(ec_integer, (exo_integer_t)thread->stat.num_insn),    \
      /*PC*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_PC),    \
      /*NPC*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_NPC),    \
      /*aflags*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_C.aflags),    \
      /*cwd*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_C.cwd),    \
      /*fsw*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_C.fsw),    \
      NULL)

#define MD_IREG_TO_EXO(REGS, IDX)                    \
    exo_new(ec_address, (exo_integer_t)(REGS)->regs_R.dw[IDX])

#define MD_FREG_TO_EXO(REGS, IDX)                    \
    exo_new(ec_address, (exo_integer_t)(REGS)->regs_F.e[IDX])

#define MD_SREG_TO_EXO(REGS, IDX)                    \
    exo_new(ec_address, (exo_integer_t)(REGS)->regs_S.w[IDX])

/* Gabe */
#define MD_EXO_TO_MISC_REGS(EXO, ICNT, REGS)                \
    /* check EXO format for errors... */                    \
  if (!exo                                \
      || exo->ec != ec_list                        \
      || !exo->as_list.head                        \
      || exo->as_list.head->ec != ec_integer                \
      || !exo->as_list.head->next                    \
      || exo->as_list.head->next->ec != ec_address            \
      || !exo->as_list.head->next->next                    \
      || exo->as_list.head->next->next->ec != ec_address        \
      || !exo->as_list.head->next->next->next                \
      || exo->as_list.head->next->next->next->ec != ec_address        \
      || !exo->as_list.head->next->next->next->next            \
      || exo->as_list.head->next->next->next->next->ec != ec_address    \
      || !exo->as_list.head->next->next->next->next->next            \
      || exo->as_list.head->next->next->next->next->next->ec != ec_address    \
      || exo->as_list.head->next->next->next->next->next->next != NULL)    \
  fatal("could not read EIO misc regs");                \
  (ICNT) = (zcounter_t)exo->as_list.head->as_integer.val;        \
  (REGS)->regs_PC = (md_addr_t)exo->as_list.head->next->as_integer.val;    \
  (REGS)->regs_NPC = (md_addr_t)exo->as_list.head->next->next->as_integer.val; \
  (REGS)->regs_C.aflags = (dword_t)exo->as_list.head->next->next->next->as_integer.val; \
  (REGS)->regs_C.cwd = (word_t)exo->as_list.head->next->next->next->next->as_integer.val; \
  (REGS)->regs_C.fsw = (word_t)exo->as_list.head->next->next->next->next->next->as_integer.val;

#define MD_EXO_TO_IREG(EXO, REGS, IDX)                    \
    ((REGS)->regs_R.dw[IDX] = (qword_t)(EXO)->as_integer.val)

#define MD_EXO_TO_FREG(EXO, REGS, IDX)                    \
    ((REGS)->regs_F.e[IDX] = (qword_t)(EXO)->as_integer.val)

#define MD_EXO_TO_SREG(EXO, REGS, IDX)                    \
    ((REGS)->regs_S.w[IDX] = (word_t)(EXO)->as_integer.val)

#define MD_EXO_CMP_IREG(EXO, REGS, IDX)                    \
    ((REGS)->regs_R.dw[IDX] != (qword_t)(EXO)->as_integer.val)

  /*
   * configure the EXO package
   */

  /* EXO pointer class */
  typedef qword_t exo_address_t;

  /* EXO integer class, 64-bit encoding */
  typedef qword_t exo_integer_t;

  /* EXO floating point class, 64-bit encoding */
  typedef double exo_float_t;


  /*
   * configure the stats package
   */

  /* counter stats */
#define stat_reg_counter        stat_reg_sqword
#define sc_counter            sc_sqword
#define for_counter            for_sqword

  /* address stats */
#define stat_reg_addr            stat_reg_uint


  /*
   * configure the DLite! debugger
   */

  /* register bank specifier */
  enum md_reg_type {
    rt_gpr,        /* general purpose register */
    rt_lpr,        /* integer-precision floating pointer register */
    rt_fpr,        /* single-precision floating pointer register */
    rt_dpr,        /* double-precision floating pointer register */
    rt_ctrl,        /* control register */
    rt_PC,        /* program counter */
    rt_NPC,        /* next program counter */
    rt_NUM
  };

/* register name specifier */
struct md_reg_names_t {
  char *str;            /* register name */
  enum md_reg_type file;    /* register file */
  int reg;            /* register index */
};

/* symbolic register names, parser is case-insensitive */
extern const struct md_reg_names_t md_reg_names[];

/* returns a register name string */
char *md_reg_name(const enum md_reg_type rt, const int reg);

/* default register accessor object */
struct eval_value_t;
struct regs_t;
char *                        /* err str, NULL for no err */
md_reg_obj(struct regs_t *regs,            /* registers to access */
    const int is_write,            /* access type */
    const enum md_reg_type rt,            /* reg bank to probe */
    const int reg,                /* register number */
    struct eval_value_t *val);        /* input, output */

/* print integer REG(S) to STREAM */
void md_print_ireg(md_gpr_t regs, int reg, FILE *stream);
void md_print_iregs(md_gpr_t regs, FILE *stream);

/* print floating point REG(S) to STREAM */
void md_print_fpreg(md_fpr_t regs, int reg, FILE *stream);
void md_print_fpregs(md_fpr_t regs, FILE *stream);

/* print control REG(S) to STREAM */
void md_print_creg(md_ctrl_t regs, int reg, FILE *stream);
void md_print_cregs(md_ctrl_t regs, FILE *stream);


/*
 * configure sim-outorder specifics
 */

/* primitive operation used to compute addresses within pipeline */
#define MD_AGEN_OP        ADDQ

/* NOP operation when injected into the pipeline */
#define MD_NOP_OP        OP_NA

/*
 * target-dependent routines
 */

/* intialize the inst decoder, this function builds the ISA decode tables */
void md_init_decoder(void);

/*
 * UOP stuff
 */

/* microcode field specifiers (xfields) */
enum md_xfield_t {
  /* integer register constants */
  XR_AL, XR_AH, XR_AX, XR_EAX, XR_eAX,
  XR_CL, XR_CH, XR_CX, XR_ECX, XR_eCX,
  XR_DL, XR_DH, XR_DX, XR_EDX, XR_eDX,
  XR_BL, XR_BH, XR_BX, XR_EBX, XR_eBX,
  XR_SP, XR_ESP, XR_eSP,
  XR_BP, XR_EBP, XR_eBP,
  XR_SI, XR_ESI, XR_eSI,
  XR_DI, XR_EDI, XR_eDI,

  /* specify NO segment register - needed for a few weird uop flows */
  XR_SEGNONE,

  /* integer temporary registers */
  XR_TMP0, XR_TMP1, XR_TMP2, XR_ZERO,

  /* FP register constants */
  XR_ST0, XR_ST1, XR_ST2, XR_ST3, XR_ST4, XR_ST5, XR_ST6, XR_ST7,

  /* FP temporary registers */
  XR_FTMP0, XR_FTMP1, XR_FTMP2,

  /* x86 bitfields */
  XF_RO, XF_R, XF_RM, XF_BASE, XF_INDEX, XF_SCALE, XF_STI,
  XF_DISP, XF_IMMB, XF_IMMV, XF_IMMA, XE_SIZEV_IMMW, XF_CC,
  XF_SYSCALL, XF_SEG,

  /* literal expressions */
  XE_ZERO, XE_ONE, XE_MONE, XE_SIZEV, XE_MSIZEV, XE_ILEN,
  XE_CCE, XE_CCNE,
  XE_FCCB, XE_FCCNB, XE_FCCE, XE_FCCNE, XE_FCCBE, XE_FCCNBE, XE_FCCU, XE_FCCNU,
  XE_CF, XE_DF, XE_SFZFAFPFCF,
  XE_FP1, XE_FNOP, XE_FPUSH, XE_FPOP, XE_FPOPPOP
};

/* UOP visibility switches */
#define UV        1ULL    /* visible */
#define UP        0ULL    /* private */
#define ISUV        ((uop[0].decode.raw_op >> 30) & 1)


/* UOP fields */
#define UHASIMM        ((uop[0].decode.raw_op >> 31) & 1)
#define USG             (UHASIMM ? (uop[2].decode.raw_op & 0xf): 0)
#define URD        ((uop[0].decode.raw_op >> 12) & 0xf)
#define UCC        URD
#define URS        ((uop[0].decode.raw_op >> 8) & 0xf)
#define URT        ((uop[0].decode.raw_op >> 4) & 0xf)
#define URU        (uop[0].decode.raw_op & 0xf)
#define ULIT        (uop[0].decode.raw_op & 0xf)
#define UIMMB                                \
  (UHASIMM                                \
   ? ((sdword_t)(sbyte_t)(uop[1].decode.raw_op & 0xff))                \
   : ((sdword_t)(sbyte_t)(uop[0].decode.raw_op & 0xff)))
#define UIMMUB                                \
  (UHASIMM                                \
   ? ((dword_t)(byte_t)(uop[1].decode.raw_op & 0xff))                    \
   : ((dword_t)(byte_t)(uop[0].decode.raw_op & 0xff)))
#define UIMMW                                \
  (UHASIMM                                \
   ? ((sdword_t)(sword_t)(uop[1].decode.raw_op & 0xffff))                \
   : ((sdword_t)(sword_t)(word_t)(uop[0].decode.raw_op & 0xff)))
#define UIMMUW                                \
  (UHASIMM                                \
   ? ((dword_t)(word_t)(uop[1].decode.raw_op & 0xffff))                \
   : ((dword_t)(word_t)(uop[0].decode.raw_op & 0xff)))
#define UIMMD                                \
  (UHASIMM                                \
   ? ((sdword_t)uop[1].decode.raw_op)                            \
   : ((sdword_t)(dword_t)(uop[0].decode.raw_op & 0xff)))
#define UIMMUD                                \
  (UHASIMM                                \
   ? ((dword_t)uop[1].decode.raw_op)                            \
   : ((dword_t)(uop[0].decode.raw_op & 0xff)))

enum fpstack_ops_t {
  fpstk_nop = 0,
  fpstk_pop = 1,
  fpstk_poppop = 2,
  fpstk_push = 3
};

/* mode-specific constants */
#define SIZE_W            2
#define SIZE_D            4
#define BITSIZE_W        16
#define BITSIZE_D        32
#define LOGSIZE_W        1
#define LOGSIZE_D        2
#define LOGBITSIZE_W        4
#define LOGBITSIZE_D        5

#define MD_SET_UOPCODE(UOP, INST)                    \
  ((UOP) = (enum md_opcode)((((INST)[0]) >> 16) & 0x3fff))

/* XXX X86 UOP definition */
#define MD_MAX_FLOWLEN        62 /* leave 2 for REP ctrl uops */
#define UOP_SEQ_SHIFT         6
typedef qword_t uop_inst_t;

#include "core/zesto-structs.h"


/* UOP flow generator, returns a small non-cyclic program implementing OP,
   returns length of flow returned */
int
md_get_flow(struct Mop_t * Mop,
    uop_inst_t flow[MD_MAX_FLOWLEN], bool * bogus);

/* disassemble a SimpleScalar instruction */
void
md_print_insn(struct Mop_t * Mop,        /* instruction to disassemble */
    FILE *stream);        /* output stream */

/* print uop function */
void
md_print_uop(const enum md_opcode op,
    const md_inst_t instruction,    /* instruction to disassemble */
    const md_addr_t pc,        /* addr of inst, used for PC-rels */
    FILE *stream);        /* output stream */

void
md_print_uop_1(struct uop_t *uop,
    md_inst_t instruction,    /* instruction to disassemble */
    md_addr_t pc,        /* addr of inst, used for PC-rels */
    FILE *stream);        /* output stream */

word_t
md_uop_opc(const enum md_opcode uopcode);

byte_t
md_uop_reg(const enum md_xfield_t xval, const struct Mop_t * Mop, bool * bogus);

byte_t
md_uop_immb(const enum md_xfield_t xval, const struct Mop_t * Mop, bool * bogus);

inline dword_t
md_uop_immv(const enum md_xfield_t xval, const struct Mop_t * Mop, bool * bogus);

inline dword_t
md_uop_lit(const enum md_xfield_t xval, const struct Mop_t * Mop, bool * bogus);

#ifdef ZESTO_ORACLE_C

#define XMEM_READ_BYTE(M,A)                        \
  ({ byte_t _mem_read_tmp;                         \
   const word_t _x = (word_t)MEM_READ_BYTE(M,A);   \
   _x; })

#define XMEM_READ_WORD(M,A)                        \
  ({ byte_t _mem_read_tmp;                         \
   const word_t _x = ((word_t)MEM_READ_BYTE(M,A)   \
     | ((word_t)MEM_READ_BYTE(M,(A)+1) << 8));     \
   _x; })

#define XMEM_READ_DWORD(M,A)                       \
  ({ byte_t _mem_read_tmp;                         \
   const dword_t _x = ((dword_t)MEM_READ_BYTE(M,A) \
     | ((dword_t)MEM_READ_BYTE(M,(A)+1) << 8)      \
     | ((dword_t)MEM_READ_BYTE(M,(A)+2) << 16)     \
     | ((dword_t)MEM_READ_BYTE(M,(A)+3) << 24));   \
   _x; })

#define XMEM_READ_QWORD(M,A)                       \
  ({ byte_t _mem_read_tmp;                         \
   const qword_t _x = ((qword_t)MEM_READ_BYTE(M,A) \
     | ((qword_t)MEM_READ_BYTE(M,(A)+1) << 8)      \
     | ((qword_t)MEM_READ_BYTE(M,(A)+2) << 16)     \
     | ((qword_t)MEM_READ_BYTE(M,(A)+3) << 24)     \
     | ((qword_t)MEM_READ_BYTE(M,(A)+4) << 32)     \
     | ((qword_t)MEM_READ_BYTE(M,(A)+5) << 40)     \
     | ((qword_t)MEM_READ_BYTE(M,(A)+6) << 48)     \
     | ((qword_t)MEM_READ_BYTE(M,(A)+7) << 56));   \
   _x; })

#define XMEM_WRITE_BYTE(M,A,S)                         \
  ({ const byte_t _x = (S);                            \
   assert(uop->oracle.spec_mem[0] == NULL);            \
   uop->oracle.spec_mem[0] = MEM_WRITE_BYTE(M,(A),_x); \
   })

#define XMEM_WRITE_WORD(M,A,S)                                       \
  ({ const word_t _x = (S);                                          \
   assert(uop->oracle.spec_mem[0] == NULL);                          \
   uop->oracle.spec_mem[0] = MEM_WRITE_BYTE(M,(A),_x&0xff);          \
   uop->oracle.spec_mem[1] = MEM_WRITE_BYTE(M,(A)+1,(_x >> 8)&0xff); \
   })

#define XMEM_WRITE_DWORD(M,A,S)                                       \
  ({ const dword_t _x = (S);                                          \
   assert(uop->oracle.spec_mem[0] == NULL);                           \
   uop->oracle.spec_mem[0] = MEM_WRITE_BYTE(M,(A),_x&0xff);           \
   uop->oracle.spec_mem[1] = MEM_WRITE_BYTE(M,(A)+1,(_x >> 8)&0xff);  \
   uop->oracle.spec_mem[2] = MEM_WRITE_BYTE(M,(A)+2,(_x >> 16)&0xff); \
   uop->oracle.spec_mem[3] = MEM_WRITE_BYTE(M,(A)+3,(_x >> 24)&0xff); \
   })

#define XMEM_WRITE_DWORD2(M,A,S)                                       \
  ({ const dword_t _x = (S);                                           \
   assert(uop->oracle.spec_mem[8] == NULL);                            \
   uop->oracle.spec_mem[8] = MEM_WRITE_BYTE(M,(A),_x&0xff);            \
   uop->oracle.spec_mem[9] = MEM_WRITE_BYTE(M,(A)+1,(_x >> 8)&0xff);   \
   uop->oracle.spec_mem[10] = MEM_WRITE_BYTE(M,(A)+2,(_x >> 16)&0xff); \
   uop->oracle.spec_mem[11] = MEM_WRITE_BYTE(M,(A)+3,(_x >> 24)&0xff); \
   })

#define XMEM_WRITE_QWORD(M,A,S)                                       \
  ({ const qword_t _x = (S);                                          \
   assert(uop->oracle.spec_mem[0] == NULL);                           \
   uop->oracle.spec_mem[0] = MEM_WRITE_BYTE(M,(A),_x&0xff);           \
   uop->oracle.spec_mem[1] = MEM_WRITE_BYTE(M,(A)+1,(_x >> 8)&0xff);  \
   uop->oracle.spec_mem[2] = MEM_WRITE_BYTE(M,(A)+2,(_x >> 16)&0xff); \
   uop->oracle.spec_mem[3] = MEM_WRITE_BYTE(M,(A)+3,(_x >> 24)&0xff); \
   uop->oracle.spec_mem[4] = MEM_WRITE_BYTE(M,(A)+4,(_x >> 32)&0xff); \
   uop->oracle.spec_mem[5] = MEM_WRITE_BYTE(M,(A)+5,(_x >> 40)&0xff); \
   uop->oracle.spec_mem[6] = MEM_WRITE_BYTE(M,(A)+6,(_x >> 48)&0xff); \
   uop->oracle.spec_mem[7] = MEM_WRITE_BYTE(M,(A)+7,(_x >> 56)&0xff); \
   })

#else

/* TODO: check alignment and then use more efficient wide accesses */

#define XMEM_READ_BYTE(M,A)                      \
  ({ const byte_t _x                             \
     = (byte_t)MEM_READ_BYTE(M,A);               \
   _x; })

#define XMEM_READ_WORD(M,A)                      \
  ({ const word_t _x                             \
     = ((word_t)MEM_READ_BYTE(M,A)               \
     | ((word_t)MEM_READ_BYTE(M,(A)+1) << 8));   \
   _x; })

#define XMEM_READ_DWORD(M,A)                     \
  ({ const dword_t _x                            \
     = ((dword_t)MEM_READ_BYTE(M,A)              \
     | ((dword_t)MEM_READ_BYTE(M,(A)+1) << 8)    \
     | ((dword_t)MEM_READ_BYTE(M,(A)+2) << 16)   \
     | ((dword_t)MEM_READ_BYTE(M,(A)+3) << 24)); \
   _x; })

#define XMEM_READ_QWORD(M,A)                     \
  ({ const qword_t _x                            \
     = ((qword_t)MEM_READ_BYTE(M,A)              \
     | ((qword_t)MEM_READ_BYTE(M,(A)+1) << 8)    \
     | ((qword_t)MEM_READ_BYTE(M,(A)+2) << 16)   \
     | ((qword_t)MEM_READ_BYTE(M,(A)+3) << 24)   \
     | ((qword_t)MEM_READ_BYTE(M,(A)+4) << 32)   \
     | ((qword_t)MEM_READ_BYTE(M,(A)+5) << 40)   \
     | ((qword_t)MEM_READ_BYTE(M,(A)+6) << 48)   \
     | ((qword_t)MEM_READ_BYTE(M,(A)+7) << 56)); \
   _x; })

#define XMEM_WRITE_WORD(M,A,S)                   \
  ({ const word_t _x = (S);                      \
   MEM_WRITE_BYTE(M,A,_x&0xff);                  \
   MEM_WRITE_BYTE(M,(A)+1,(_x >> 8)&0xff); })

#define XMEM_WRITE_DWORD(M,A,S)                  \
  ({ const dword_t _x = (S);                     \
   MEM_WRITE_BYTE(M,A,_x&0xff);                  \
   MEM_WRITE_BYTE(M,(A)+1,(_x >> 8)&0xff);       \
   MEM_WRITE_BYTE(M,(A)+2,(_x >> 16)&0xff);      \
   MEM_WRITE_BYTE(M,(A)+3,(_x >> 24)&0xff); })

#define XMEM_WRITE_QWORD(M,A,S)                  \
  ({ const qword_t _x = (S);                     \
   MEM_WRITE_BYTE(M,A,_x&0xff);                  \
   MEM_WRITE_BYTE(M,(A)+1,(_x >> 8)&0xff);       \
   MEM_WRITE_BYTE(M,(A)+2,(_x >> 16)&0xff);      \
   MEM_WRITE_BYTE(M,(A)+3,(_x >> 24)&0xff);      \
   MEM_WRITE_BYTE(M,(A)+4,(_x >> 32)&0xff);      \
   MEM_WRITE_BYTE(M,(A)+5,(_x >> 40)&0xff);      \
   MEM_WRITE_BYTE(M,(A)+6,(_x >> 48)&0xff);      \
   MEM_WRITE_BYTE(M,(A)+7,(_x >> 56)&0xff); })

#endif /* ZESTO_ORACLE_C */

#define READ_V(ADDR, FAULT)                         \
  ((Mop->fetch.inst.mode & MODE_OPER32)             \
   ? ((dword_t)READ_DWORD((ADDR), (FAULT)))         \
   : ((dword_t)(word_t)READ_WORD((ADDR), (FAULT))))
#define WRITE_V(SRC, ADDR, FAULT)                   \
  ((Mop->fetch.inst.mode & MODE_OPER32)             \
   ? WRITE_DWORD((SRC), (ADDR), (FAULT))            \
   : WRITE_WORD((SRC), (ADDR), (FAULT)))
#define READ_S2E(ADDR, FAULT)                       \
  ({ union { sfloat_t s; dword_t dw; } x;           \
   x.dw = READ_DWORD((ADDR), (FAULT));              \
   (efloat_t)x.s;                                   \
   })
#define WRITE_E2S(SRC, ADDR, FAULT)                 \
  ({ union { sfloat_t s; dword_t dw; } x;           \
   x.s = (sfloat_t)(SRC);                           \
   WRITE_DWORD(x.dw, (ADDR), (FAULT));              \
   })
#define READ_T2E(ADDR, FAULT)                       \
  ({ union { dfloat_t d; qword_t q; } x;            \
   x.q = READ_QWORD((ADDR), (FAULT));               \
   (efloat_t)x.d;                                   \
   })
#define WRITE_E2T(SRC, ADDR, FAULT)                 \
  ({ union { dfloat_t d; qword_t q; } x;            \
   x.d = (dfloat_t)(SRC);                           \
   WRITE_QWORD(x.q, (ADDR), (FAULT));               \
   })
#define READ_Q2E(ADDR, FAULT)                       \
  ({ const qword_t q                                \
     = READ_QWORD((ADDR), (FAULT));                 \
   (efloat_t)q;                                     \
   })
#define WRITE_E2Q(SRC, ADDR, FAULT)                 \
  ({ const qword_t q                                \
     = (qword_t)(SRC);                              \
   WRITE_QWORD(q, (ADDR), (FAULT));                 \
   })
#define READ_D2E(ADDR, FAULT)                       \
  ({ const dword_t dw                               \
     = READ_DWORD((ADDR), (FAULT));                 \
   (efloat_t)(sdword_t)dw;                          \
   })
#define WRITE_E2D(SRC, ADDR, FAULT)                 \
  ({ const dword_t dw                               \
     = (sdword_t)(SRC);                             \
   WRITE_DWORD(dw, (ADDR), (FAULT));                \
   })

#define READ_W2E(ADDR, FAULT)                       \
  ({ const word_t w                                 \
     = READ_WORD((ADDR), (FAULT));                  \
   (efloat_t)w;                                     \
   })
#define WRITE_E2W(SRC, ADDR, FAULT)                 \
  ({ const word_t w                                 \
     = (word_t)(SRC);                               \
   WRITE_WORD(w, (ADDR), (FAULT));                  \
   })


#define REP_COUNT                                       \
  ((Mop->fetch.inst.rep)                                \
   ? ((Mop->fetch.inst.mode & MODE_ADDR32)              \
     ? (SET_GPR_D(MD_REG_ECX, (GPR_D(MD_REG_ECX)) - 1)) \
     : (SET_GPR_W(MD_REG_CX, (GPR_W(MD_REG_CX)) - 1)))  \
   : (FALSE))

#define REP_UNCOUNT                                     \
  ((Mop->fetch.inst.rep)                                \
   ? ((Mop->fetch.inst.mode & MODE_ADDR32)              \
     ? (SET_GPR_D(MD_REG_ECX, (GPR_D(MD_REG_ECX)) + 1)) \
     : (SET_GPR_W(MD_REG_CX, (GPR_W(MD_REG_CX)) + 1)))  \
   : (FALSE))

#define REP_REMAINING                                   \
  (Mop->fetch.inst.rep                                  \
   ? ((Mop->fetch.inst.mode & MODE_ADDR32)              \
     ? (GPR_D(MD_REG_ECX))                              \
     : (GPR_W(MD_REG_CX)) )                             \
   : (0)) 

// TRUE for repeat and non-zero repetitions or non-repeat
// FALSE for repeat instructions with zero repetitions
#define REP_FIRST(regs)                                            \
  (!(Mop->fetch.inst.rep && !((Mop->fetch.inst.mode & MODE_ADDR32) \
                              ? (regs.regs_R.dw[MD_REG_ECX])       \
                              : (regs.regs_R.w[MD_REG_CX].lo))))

// TRUE for repeat and stop condition not met
// FALSE for non-repeat or repeat with stop-condition met
#define REP_AGAIN(regs)                                                     \
  ((Mop->fetch.inst.rep)                                                    \
   ? ((Mop->fetch.inst.rep == REP_REPNZ)                                    \
     ? (((Mop->fetch.inst.mode & MODE_ADDR32)                               \
         ? (regs.regs_R.dw[MD_REG_ECX] && !(regs.regs_C.aflags & ZF))       \
         : (regs.regs_R.w[MD_REG_CX].lo && !(regs.regs_C.aflags & ZF))))    \
     : (((Mop->fetch.inst.rep == REP_REPZ)                                  \
         ? (((Mop->fetch.inst.mode & MODE_ADDR32)                           \
             ? (regs.regs_R.dw[MD_REG_ECX] && (regs.regs_C.aflags & ZF))    \
             : (regs.regs_R.w[MD_REG_CX].lo && (regs.regs_C.aflags & ZF)))) \
         : (((Mop->fetch.inst.rep == REP_REP)                               \
             ? (((Mop->fetch.inst.mode & MODE_ADDR32)                       \
                 ? (regs.regs_R.dw[MD_REG_ECX])                             \
                 : (regs.regs_R.w[MD_REG_CX].lo)))                          \
             : ((panic("bogus repeat code")),0))))))                        \
   : (FALSE))

#undef IE
#undef DE
#undef ZE
#undef OE
#undef UE
#undef PE
#undef FSF
#undef ES
#undef C0
#undef C1
#undef C2
#undef C3

#undef R //avoid conflict with boost library
#undef RO
#undef RM

#ifdef __cplusplus
}
#else
#undef bool
#endif

#endif /* X86_H */



