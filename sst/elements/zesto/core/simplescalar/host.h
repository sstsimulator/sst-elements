/* host.h - host-dependent definitions and interfaces */

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

#ifndef HOST_H
#define HOST_H

/* make sure host compiler supports ANSI-C */
#ifndef __STDC__ /* an ansi C compiler is required */
#error The SimpleScalar simulators must be compiled with an ANSI C compiler.
#endif /* __STDC__ */

/* enable inlining here, if supported by host compiler */
#undef INLINE
#if defined(__GNUC__)
#define INLINE		inline
#else
#define INLINE
#endif

/* bind together two symbols, at preprocess time */
#ifdef __GNUC__
/* this works on all GNU GCC targets (that I've seen...) */
#ifndef SYMCAT
#define SYMCAT(X,Y)	X##Y
#endif

#define ANSI_SYMCAT
#else /* !__GNUC__ */
#ifdef OLD_SYMCAT

#ifndef SYMCAT
#define SYMCAT(X,Y)	X/**/Y
#endif

#else /* !OLD_SYMCAT */
#ifndef SYMCAT
#define SYMCAT(X,Y)	X##Y
#endif

#define ANSI_SYMCAT
#endif /* OLD_SYMCAT */
#endif /* __GNUC__ */

/* host-dependent canonical type definitions */
typedef int bool_t;			/* generic boolean type */
typedef unsigned short half_t;
typedef unsigned char byte_t;		/* byte - 8 bits */
typedef signed char sbyte_t;
typedef unsigned short word_t;		/* word - 16 bits */
typedef signed short sword_t;
typedef unsigned int dword_t;		/* double word - 32 bits */
typedef signed int sdword_t;
typedef float sfloat_t;			/* single-precision float - 32 bits */
typedef double dfloat_t;		/* double-precision float - 64 bits */
typedef long double efloat_t;		/* extended-precision float - varies */

/* qword defs, note: not all targets support qword types */
#if defined(__GNUC__) || defined(__SUNPRO_C) || defined(__CC_C89) || defined(__CC_XLC)
#define HOST_HAS_QWORD
typedef unsigned long long qword_t;	/* qword - 64 bits */
typedef signed long long int sqword_t;
#ifdef ANSI_SYMCAT
#define ULL(N)		N##ULL		/* qword_t constant */
#define LL(N)		N##LL		/* sqword_t constant */
#else /* OLD_SYMCAT */
#define ULL(N)		N/**/ULL	/* qword_t constant */
#define LL(N)		N/**/LL		/* sqword_t constant */
#endif
#elif defined(__alpha)
#define HOST_HAS_QWORD
typedef unsigned long qword_t;		/* qword - 64 bits */
typedef signed long sqword_t;
#ifdef ANSI_SYMCAT
#define ULL(N)		N##UL		/* qword_t constant */
#define LL(N)		N##L		/* sqword_t constant */
#else /* OLD_SYMCAT */
#define ULL(N)		N/**/UL		/* qword_t constant */
#define LL(N)		N/**/L		/* sqword_t constant */
#endif
#elif defined(_MSC_VER)
#define HOST_HAS_QWORD
typedef unsigned __int64 qword_t;	/* qword - 64 bits */
typedef signed __int64 sqword_t;
#define ULL(N)		((qword_t)(N))
#define LL(N)		((sqword_t)(N))
#else /* !__GNUC__ && !__alpha */
#undef HOST_HAS_QWORD
#endif

/* statistical counter types, use largest counter type available */
#ifdef HOST_HAS_QWORD
typedef sqword_t zcounter_t;
#else /* !HOST_HAS_QWORD */
typedef dfloat_t zcounter_t;
#endif /* HOST_HAS_QWORD */

#ifdef __svr4__
#define setjmp	_setjmp
#define longjmp	_longjmp
#endif

#endif /* HOST_H */
