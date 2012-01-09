/* regs.h - architected register state interfaces */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2002 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
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
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 1994-2002 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */

#ifndef REGS_H
#define REGS_H

#include "host.h"
#include "misc.h"
#include "machine.h"

/*
 * This module implements the SimpleScalar architected register state, which
 * includes integer and floating point registers and miscellaneous registers.
 * The architected register state is as follows:
 *
 * Integer Register File:                  Miscellaneous Registers:
 * (aka general-purpose registers, GPR's)
 *
 *   +------------------+                  +------------------+
 *   | $r0 (src/sink 0) |                  | PC               | Program Counter
 *   +------------------+                  +------------------+
 *   | $r1              |                  | HI               | Mult/Div HI val
 *   +------------------+                  +------------------+
 *   |  .               |                  | LO               | Mult/Div LO val
 *   |  .               |                  +------------------+
 *   |  .               |
 *   +------------------+
 *   | $r31             |
 *   +------------------+
 *
 * Floating point Register File:
 *    single-precision:  double-precision:
 *   +------------------+------------------+  +------------------+
 *   | $f0              | $f1 (for double) |  | FCC              | FP codes
 *   +------------------+------------------+  +------------------+
 *   | $f1              |
 *   +------------------+
 *   |  .               |
 *   |  .               |
 *   |  .               |
 *   +------------------+------------------+
 *   | $f30             | $f31 (for double)|
 *   +------------------+------------------+
 *   | $f31             |
 *   +------------------+
 *
 * The floating point register file can be viewed as either 32 single-precision
 * (32-bit IEEE format) floating point values $f0 to $f31, or as 16
 * double-precision (64-bit IEEE format) floating point values $f0 to $f30.
 */

struct regs_t {
  md_gpr_t regs_R;		/* (signed) integer register file */
  md_fpr_t regs_F;		/* floating point register file */
  md_ctrl_t regs_C;		/* control register file */
  md_addr_t regs_PC;		/* program counter */
  md_addr_t regs_NPC;		/* next-cycle program counter */
  md_seg_t regs_S;  		/* segment register file */ // UCSD
};

/* create a register file */
struct regs_t *regs_create(void);

/* initialize architected register state */
void
regs_init(struct regs_t *regs);		/* register file to initialize */

#endif /* REGS_H */
