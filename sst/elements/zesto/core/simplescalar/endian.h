/* endian.h - host endian probe interfaces */

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

#ifndef ENDIAN_H
#define ENDIAN_H

#include "thread.h"

/* data swapping functions, from big/little to little/big endian format */
#define SWAP_WORD(X)							\
  (((((word_t)(X)) & 0xff) << 8) | ((((word_t)(X)) & 0xff00) >> 8))
#define SWAP_DWORD(X)	(((dword_t)(X) << 24) |				\
			 (((dword_t)(X) << 8)  & 0x00ff0000) |		\
			 (((dword_t)(X) >> 8)  & 0x0000ff00) |		\
			 (((dword_t)(X) >> 24) & 0x000000ff))
#define SWAP_QWORD(X)	(((qword_t)(X) << 56) |				\
			 (((qword_t)(X) << 40) & ULL(0x00ff000000000000)) |\
			 (((qword_t)(X) << 24) & ULL(0x0000ff0000000000)) |\
			 (((qword_t)(X) << 8)  & ULL(0x000000ff00000000)) |\
			 (((qword_t)(X) >> 8)  & ULL(0x00000000ff000000)) |\
			 (((qword_t)(X) >> 24) & ULL(0x0000000000ff0000)) |\
			 (((qword_t)(X) >> 40) & ULL(0x000000000000ff00)) |\
			 (((qword_t)(X) >> 56) & ULL(0x00000000000000ff)))

/* recognized endian formats */
enum endian_t { endian_big, endian_little, endian_unknown};
/* probe host (simulator) byte endian format */
enum endian_t
endian_host_byte_order(void);

/* probe host (simulator) double word endian format */
enum endian_t
endian_host_word_order(void);

#ifndef HOST_ONLY

/* probe target (simulated program) byte endian format, only
   valid after program has been loaded */
enum endian_t
endian_target_byte_order(struct thread_t * core);

/* probe target (simulated program) double word endian format,
   only valid after program has been loaded */
enum endian_t
endian_target_word_order(struct thread_t * core);

#endif /* HOST_ONLY */

#endif /* ENDIAN_H */
