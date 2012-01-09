/* misc.h - miscellaneous interfaces
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

#ifndef MISC_H
#define MISC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

/* boolean value defs */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* various useful macros */
#ifndef MAX
#define MAX(a, b)    (((a) < (b)) ? (b) : (a))
#endif
#ifndef MIN
#define MIN(a, b)    (((a) < (b)) ? (a) : (b))
#endif

/* for printing out "long long" vars */
#define LLHIGH(L)		((int)(((L)>>32) & 0xffffffff))
#define LLLOW(L)		((int)((L) & 0xffffffff))

/* size of an array, in elements */
#define N_ELT(ARR)   (sizeof(ARR)/sizeof((ARR)[0]))

/* rounding macros, assumes ALIGN is a power of two */
#define ROUND_UP(N,ALIGN)	(((N) + ((ALIGN)-1)) & ~((ALIGN)-1))
#define ROUND_DOWN(N,ALIGN)	((N) & ~((ALIGN)-1))

#ifdef DEBUG
/* active debug flag */
extern bool debugging;
#endif /* DEBUG */

/* register a function to be called when an error is detected */
void
fatal_hook(void (*hook_fn)(FILE *stream));	/* fatal hook function */

#ifdef __GNUC__
/* declare a fatal run-time error, calls fatal hook function */
#define fatal(fmt, args...)	\
  _fatal(__FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_fatal(const char *file, const char *func, const int line, const char *fmt, ...)
__attribute__ ((noreturn));
#else /* !__GNUC__ */
void
fatal(const char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef __GNUC__
/* declare a panic situation, dumps core */
#define panic(fmt, args...)	\
  _panic(__FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_panic(const char *file, const char *func, const int line, const char *fmt, ...)
__attribute__ ((noreturn));
#else /* !__GNUC__ */
void
panic(const char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef __GNUC__
/* declare a warning */
#define warn(fmt, args...)	\
  _warn(__FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_warn(const char *file, const char *func, const int line, const char *fmt, ...);
#else /* !__GNUC__ */
void
warn(const char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef __GNUC__
/* declare a oneshot warning */
#define warnonce(fmt, args...)						\
  do {									\
    static int __first = TRUE;						\
    if (__first) _warn(__FILE__, __FUNCTION__, __LINE__, fmt, ## args);	\
    __first = FALSE;							\
  } while (0)
void
_warn(const char *file, const char *func, const int line, const char *fmt, ...);
#else /* !__GNUC__ */
void
warn(const char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef __GNUC__
/* print general information */
#define info(fmt, args...)	\
  _info(__FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_info(const char *file, const char *func, const int line, const char *fmt, ...);
#else /* !__GNUC__ */
void
info(const char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef DEBUG

#ifdef __GNUC__
/* print a debugging message */
#define debug(fmt, args...)	\
    do {                        \
        if (debugging)         	\
            _debug(__FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
    } while(0)

void
_debug(const char *file, const char *func, const int line, const char *fmt, ...);
#else /* !__GNUC__ */
void
debug(const char *fmt, ...);
#endif /* !__GNUC__ */

#else /* !DEBUG */

#ifdef __GNUC__
#define debug(fmt, args...)
#else /* !__GNUC__ */
/* the optimizer should eliminate this call! */
static void debug(const char *fmt, ...) {}
#endif /* !__GNUC__ */

#endif /* !DEBUG */

/* seed the random number generator */
void
mysrand(const unsigned int seed);	/* random number generator seed */

/* get a random number */
int myrand(void);		/* returns random number */

/* ctype.h replacements */
char mytoupper(char c);
char mytolower(char c);
bool myisalpha(char c);
bool myisdigit(char c);
bool myisspace(char c);
bool myisprint(char c);

/* copy a string to a new storage allocation (NOTE: many machines are missing
   this trivial function, so I funcdup() it here...) */
char *				/* duplicated string */
mystrdup(const char *s);		/* string to duplicate to heap storage */

/* find the last occurrence of a character in a string */
const char *
mystrrchr(const char *s, const char c);

/* case insensitive string compare (NOTE: many machines are missing this
   trivial function, so I funcdup() it here...) */
int				/* compare result, see strcmp() */
mystricmp(const char *s1, const char *s2);	/* strings to compare, case insensitive */

/* return log of a number to the base 2 */
int log_base2(const int n);

/* return string describing elapsed time, passed in SEC in seconds */
char *elapsed_time(long sec);

/* portable 64-bit I/O package */

/* portable vsprintf with qword support, returns end pointer */
char *myvsprintf(char *obuf, const char *format, va_list v);

/* portable sprintf with qword support, returns end pointer */
char *mysprintf(char *obuf, const char *format, ...);

/* portable vfprintf with qword support, returns end pointer */
void myvfprintf(FILE *stream, const char *format, va_list v);

/* portable fprintf with qword support, returns end pointer */
void myfprintf(FILE *stream, const char *format, ...);

#ifdef HOST_HAS_QWORD

/* convert a string to a signed result */
sqword_t myatosq(char *nptr, char **endp, int base);

/* convert a string to a unsigned result */
qword_t myatoq(char *nptr, char **endp, int base);

#endif /* HOST_HAS_QWORD */

/* same semantics as fopen() except that filenames ending with a ".gz" or ".Z"
   will be automagically get compressed */
FILE *gzopen(const char *fname, const char *type);

/* close compressed stream */
void gzclose(FILE *fd);

/* fast modulo increment/decrement:
   gcc on -O1 or higher will if-convert the following functions
   to provide much cheaper implementations of increment and
   decrement modulo some arbitrary value (not necessarily a
   power of two).  NOTE: argument x *must* be in the range
   0 .. (m-1), otherwise this code will not work! */

/* returns (x+1)%m */
inline int modinc(int x, int m)
{
  int xinc = x+1;
  if(xinc==m)
    xinc = 0;
  return xinc;
}

/* returns (x-1+m)%m */
inline int moddec(int x, int m)
{
  int xdec = x-1;
  if(x==0)
    xdec = m-1;
  return xdec;
}

/* returns x%m; NOTE: x must be in the range 0 .. (2m-1) */
inline int mod2m(int x, int m)
{
  int ret = x;
  if(x >= m)
    ret = ret - m;
  return ret;
}

/* fast memset macros - uses GCC's inline assembly */
void memzero(void * base, int bytes);
void clear_page(void * base);
void memswap(void * p1, void * p2, size_t num_bytes);

#ifdef __cplusplus
}
#endif

#endif /* MISC_H */
