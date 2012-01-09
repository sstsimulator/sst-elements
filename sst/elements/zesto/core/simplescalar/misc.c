/* misc.c - miscellaneous routines
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#if defined(linux)
#include <unistd.h>
#endif

#include "host.h"
#include "misc.h"
#include "machine.h"

#ifdef DEBUG
/* active debug flag */
bool debugging = false;
#endif /* DEBUG */

/* fatal function hook, this function is called just before an exit
   caused by a fatal error, used to spew stats, etc. */
static void (*hook_fn)(FILE *stream) = NULL;

/* register a function to be called when an error is detected */
void
fatal_hook(void (*fn)(FILE *stream))	/* fatal hook function */
{
  hook_fn = fn;
}

/* declare a fatal run-time error, calls fatal hook function */
#ifdef __GNUC__
void
_fatal(const char *file, const char *func, const int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
fatal(const char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(stderr, "fatal: ");
  myvfprintf(stderr, fmt, v);
#ifdef __GNUC__
  fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
  if (hook_fn)
    (*hook_fn)(stderr);
  fflush(stderr);
  exit(1);
}

/* declare a panic situation, dumps core */
#ifdef __GNUC__
void
_panic(const char *file, const char *func, const int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
panic(const char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(stderr, "panic: ");
  myvfprintf(stderr, fmt, v);
#ifdef __GNUC__
  fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
  if (hook_fn)
    (*hook_fn)(stderr);
  fflush(stderr);
  abort();
}

/* declare a warning */
#ifdef __GNUC__
void
_warn(const char *file, const char *func, const int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
warn(const char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(stderr, "warning: ");
  myvfprintf(stderr, fmt, v);
#ifdef __GNUC__
  fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
  fflush(stderr);
}

/* print general information */
#ifdef __GNUC__
void
_info(const char *file, const char *func, const int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
info(const char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  myvfprintf(stderr, fmt, v);
#ifdef __GNUC__
  fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
  fflush(stderr);
}

#ifdef DEBUG
/* print a debugging message */
#ifdef __GNUC__
void
_debug(const char *file, const char *func, const int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
debug(const char *fmt, ...)
#endif /* __GNUC__ */
{
    va_list v;
    va_start(v, fmt);

    if (debugging)
      {
        fprintf(stderr, "debug: ");
        myvfprintf(stderr, fmt, v);
#ifdef __GNUC__
        fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif
        fprintf(stderr, "\n");
      }
    fflush(stderr);
}
#endif /* DEBUG */


/* ctype.h replacements - response to request from Prof. Todd Austin for
   an implementation that does not use the built-in ctype.h due to
   thread-safety/reentrancy issues. */
char mytoupper(char c)
{
  int delta = 'a'-'A';
  if((c >= 'a') && (c <= 'z'))
    return c-delta;
  else
    return c;
}

char mytolower(char c)
{
  int delta = 'a'-'A';
  if((c >= 'A') && (c <= 'Z'))
    return c+delta;
  else
    return c;
}

bool myisalpha(char c)
{
  return (((c >= 'A') && (c <= 'Z')) ||
          ((c >= 'a') && (c <= 'z')));
}

bool myisdigit(char c)
{
  return ((c >= '0') && (c <= '9'));
}

bool myisspace(char c)
{
  return ((c == ' ') ||
      ((c >= 9) && (c <= 13))
      );
}

bool myisprint(char c)
{
  return (myisspace(c) || (c >= 32) && (c <= 126));
}



/* seed the random number generator */
void
mysrand(const unsigned int seed)	/* random number generator seed */
{
#if defined(__CYGWIN32__) || defined(hpux) || defined(__hpux) || defined(__svr4__) || defined(_MSC_VER)
      srand(seed);
#else
      srandom(seed);
#endif
}

/* get a random number */
int
myrand(void)			/* returns random number */
{
#if !defined(__alpha) && !defined(linux) && !defined(__CYGWIN32__)
  extern long random(void);
#endif

#if defined(__CYGWIN32__) || defined(hpux) || defined(__hpux) || defined(__svr4__) || defined(_MSC_VER)
  return rand();
#else
  return random();
#endif
}

/* copy a string to a new storage allocation (NOTE: many machines are missing
   this trivial function, so I funcdup() it here...) */
char *				/* duplicated string */
mystrdup(const char *s)		/* string to duplicate to heap storage */
{
  char *buf;

  if (!(buf = (char *)calloc(strlen(s)+1,1)))
    return NULL;
  strcpy(buf, s);
  return buf;
}

/* find the last occurrence of a character in a string */
const char *
mystrrchr(const char *s, const char c)
{
  const char *rtnval = 0;

  do {
    if (*s == c)
      rtnval = s;
  } while (*s++);

  return rtnval;
}

/* case insensitive string compare (NOTE: many machines are missing this
   trivial function, so I funcdup() it here...) */
int				/* compare result, see strcmp() */
mystricmp(const char *s1, const char *s2)	/* strings to compare, case insensitive */
{
  unsigned char u1, u2;

  for (;;)
    {
      u1 = (unsigned char)*s1++; u1 = mytolower(u1);
      u2 = (unsigned char)*s2++; u2 = mytolower(u2);

      if (u1 != u2)
	return u1 - u2;
      if (u1 == '\0')
	return 0;
    }
}

/* return log of a number to the base 2 */
int
log_base2(const int n)
{
  int power = 0;

  if (n <= 0 || (n & (n-1)) != 0)
  {
    return (int)ceil(log(n)/log(2.0));
  }

  int nn = n;
  while (nn >>= 1)
    power++;

  return power;
}

/* return string describing elapsed time, passed in SEC in seconds */
char *
elapsed_time(long sec)
{
  static char tstr[256];
  char temp[256];

  if (sec <= 0)
    return (char*)"0s";

  tstr[0] = '\0';

  /* days */
  if (sec >= 86400)
    {
      sprintf(temp, "%ldD ", sec/86400);
      strcat(tstr, temp);
      sec = sec % 86400;
    }
  /* hours */
  if (sec >= 3600)
    {
      sprintf(temp, "%ldh ", sec/3600);
      strcat(tstr, temp);
      sec = sec % 3600;
    }
  /* mins */
  if (sec >= 60)
    {
      sprintf(temp, "%ldm ", sec/60);
      strcat(tstr, temp);
      sec = sec % 60;
    }
  /* secs */
  if (sec >= 1)
    {
      sprintf(temp, "%lds ", sec);
      strcat(tstr, temp);
    }
  tstr[strlen(tstr)-1] = '\0';
  return tstr;
}

#define PUT(p, n)							\
  {									\
    int nn, cc;								\
									\
    for (nn = 0; nn < n; nn++)						\
      {									\
	cc = *(p+nn);							\
        *obuf++ = cc;							\
      }									\
  }

#define PAD(s, n)							\
  {									\
    int nn, cc;								\
									\
    cc = *s;								\
    for (nn = n; nn > 0; nn--)						\
      *obuf++ = cc;							\
  }

#ifdef HOST_HAS_QWORD
#define HIBITL		LL(0x8000000000000000)
typedef sqword_t slargeint_t;
typedef qword_t largeint_t;
#else /* !HOST_HAS_QWORD */
#define HIBITL		0x80000000L
typedef sdword_t slargeint_t;
typedef dword_t largeint_t;
#endif /* HOST_HAS_QWORD */

static int
_lowdigit(slargeint_t *valptr)
{
  /* this function computes the decimal low-order digit of the number pointed
     to by valptr, and returns this digit after dividing *valptr by ten; this
     function is called ONLY to compute the low-order digit of a long whose
     high-order bit is set */

  int lowbit = (int)(*valptr & 1);
  slargeint_t value = (*valptr >> 1) & ~HIBITL;

  *valptr = value / 5;
  return (int)(value % 5 * 2 + lowbit + '0');
}

/* portable vsprintf with qword support, returns end pointer */
char *
myvsprintf(char *obuf, const char *format, va_list v)
{
  static char _blanks[] = "                    ";
  static char _zeroes[] = "00000000000000000000";

  /* counts output characters */
  int count = 0;

  /* format code */
  int fcode;

  /* field width and precision */
  int width, prec = 0;

  /* number of padding zeroes required on the left and right */
  int lzero = 0;

  /* length of prefix */
  int prefixlength;

  /* combined length of leading zeroes, trailing zeroes, and suffix */
  int otherlength;

  /* format flags */
#define PADZERO		0x0001	/* padding zeroes requested via '0' */
#define RZERO		0x0002	/* there will be trailing zeros in output */
#define LZERO		0x0004	/* there will be leading zeroes in output */
#define DOTSEEN		0x0008	/* dot appeared in format specification */
#define LENGTH		0x0010	/* l */
  int flagword;

  /* maximum number of digits in printable number */
#define MAXDIGS		22

  /* starting and ending points for value to be printed */
  char *bp, *p;

  /* work variables */
  int k, lradix, mradix;

  /* pointer to sign, "0x", "0X", or empty */
  char *prefix = NULL;

  /* values are developed in this buffer */
  static char buf[MAXDIGS*4], buf1[MAXDIGS*4];

  /* pointer to a translate table for digits of whatever radix */
  const char *tab;

  /* value being converted, if integer */
  slargeint_t val;

  /* value being converted, if floating point */
  dfloat_t fval;

  for (;;)
    {
      int n;

      while ((fcode = *format) != '\0' && fcode != '%')
	{
	  *obuf++ = fcode;
	  format++;
	  count++;
	}

      if (fcode == '\0')
	{
	  /* end of format; terminate and return */
	  *obuf = '\0';
	  return obuf;
	}


      /* % has been found, the following switch is used to parse the format
	 specification and to perform the operation specified by the format
	 letter; the program repeatedly goes back to this switch until the
	 format letter is encountered */

      width = prefixlength = otherlength = flagword = 0;
      format++;

    charswitch:
      switch (fcode = *format++)
	{
	case '0': /* means pad with leading zeros */
	  flagword |= PADZERO;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  {
	    int num = fcode - '0';
	    while (myisdigit(fcode = *format))
	      {
		num = num * 10 + fcode - '0';
		format++;
	      }
	    if (flagword & DOTSEEN)
	      prec = num;
	    else
	      width = num;
	    goto charswitch;
	  }

	case '.':
	  flagword |= DOTSEEN;
	  goto charswitch;

	case 'l':
	  flagword |= LENGTH;
	  goto charswitch;

	case 'n': /* host counter */
#ifdef HOST_HAS_QWORD
	  flagword |= LENGTH;
	  /* fallthru */
#else /* !HOST_HAS_QWORD */
	  flagword |= DOTSEEN;
	  if (!width)
	    width = 12;
	  prec = 0;
	  goto process_float;
#endif /* HOST_HAS_QWORD */
	  
	case 'd':
	  /* fetch the argument to be printed */
	  if (flagword & LENGTH)
	    val = va_arg(v, slargeint_t);
	  else
	    val = (slargeint_t)va_arg(v, sdword_t);

	  /* set buffer pointer to last digit */
	  p = bp = buf + MAXDIGS;

	  /* If signed conversion, make sign */
	  if (val < 0)
	    {
	      prefix = (char*)"-";
	      prefixlength = 1;
	      /* negate, checking in advance for possible overflow */
	      if (val != (slargeint_t)HIBITL)
		val = -val;
	      else
		{
		  /* number is -HIBITL; convert last digit and get pos num */
		  *--bp = _lowdigit(&val);
		}
	    }

	decimal:
	  {
	    slargeint_t qval = val;

	    if (qval <= 9)
	      *--bp = (int)qval + '0';
	    else
	      {
		do {
		  n = (int)qval;
		  qval /= 10;
		  *--bp = n - (int)qval * 10 + '0';
		}
		while (qval > 9);
		*--bp = (int)qval + '0';
	      }
	  }
	  break;

	case 'u':
	  /* fetch the argument to be printed */
	  if (flagword & LENGTH)
	    val = va_arg(v, largeint_t);
	  else
	    val = (largeint_t)va_arg(v, dword_t);

	  /* set buffer pointer to last digit */
	  p = bp = buf + MAXDIGS;

	  if (val & HIBITL)
	    *--bp = _lowdigit(&val);
	  goto decimal;

	case 'o':
	  mradix = 7;
	  lradix = 2;
	  goto fixed;

	case 'p': /* target address */
	  if (sizeof(md_addr_t) > 4)
	    flagword |= LENGTH;
	  /* fallthru */

	case 'X':
	case 'x':
	  mradix = 15;
	  lradix = 3;

	fixed:
	  /* fetch the argument to be printed */
	  if (flagword & LENGTH)
	    val = va_arg(v, largeint_t);
	  else
	    val = (largeint_t)va_arg(v, dword_t);

	  /* set translate table for digits */
	  tab = (fcode == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";

	  /* develop the digits of the value */
	  p = bp = buf + MAXDIGS;

	  {
	    slargeint_t qval = val;

	    if (qval == 0)
	      {
		otherlength = lzero = 1;
		flagword |= LZERO;
	      }
	    else
	      do {
		*--bp = tab[qval & mradix];
		qval = ((qval >> 1) & ~HIBITL) >> lradix;
	      } while (qval != 0);
	  }
	  break;

#ifndef HOST_HAS_QWORD
	process_float:
#endif /* !HOST_HAS_QWORD */

	case 'f':
	  if (flagword & DOTSEEN)
	    sprintf(buf1, "%%%d.%df", width, prec);
	  else if (width)
	    sprintf(buf1, "%%%df", width);
	  else
	    sprintf(buf1, "%%f");

	  /* fetch the argument to be printed */
	  fval = va_arg(v, dfloat_t);

	  /* print floating point value */
	  sprintf(buf, buf1, fval);
	  bp = buf;
	  p = bp + strlen(bp);
	  break;

	case 's':
	  bp = va_arg(v, char *);
	  if (bp == NULL)
	    bp = (char*)"(null)";
	  p = bp + strlen(bp);
	  break;

	case '%':
	  buf[0] = fcode;
	  goto c_merge;

	case 'c':
	  buf[0] = va_arg(v, int);
	c_merge:
	  p = (bp = &buf[0]) + 1;
	  break;

	default:
	  /* this is technically an error; what we do is to back up the format
             pointer to the offending char and continue with the format scan */
	  format--;
	  continue;
	}

      /* calculate number of padding blanks */
      k = (n = p - bp) + prefixlength + otherlength;
      if (width <= k)
	count += k;
      else
	{
	  count += width;

	  /* set up for padding zeroes if requested; otherwise emit padding
             blanks unless output is to be left-justified */
	  if (flagword & PADZERO)
	    {
	      if (!(flagword & LZERO))
		{
		  flagword |= LZERO;
		  lzero = width - k;
		}
	      else
		lzero += width - k;

	      /* cancel padding blanks */
	      k = width;
	    }
	  else
	    {
	      /* blanks on left if required */
	      PAD(_blanks, width - k);
	    }
	}

      /* prefix, if any */
      if (prefixlength != 0)
	{
	  PUT(prefix, prefixlength);
	}

      /* zeroes on the left */
      if (flagword & LZERO)
	{
	  PAD(_zeroes, lzero);
	}

      /* the value itself */
      if (n > 0)
	{
	  PUT(bp, n);
	}
    }
}

/* portable sprintf with qword support, returns end pointer */
char *
mysprintf(char *obuf, const char *format, ...)
{
  /* vararg parameters */
  va_list v;
  va_start(v, format);

  return myvsprintf(obuf, format, v);
}

/* portable vfprintf with qword support, returns end pointer */
void
myvfprintf(FILE *stream, const char *format, va_list v)
{
  /* temp buffer */
  char buf[2048];

  myvsprintf(buf, format, v);
  fputs(buf, stream);
}

/* portable fprintf with qword support, returns end pointer */
void
myfprintf(FILE *stream, const char *format, ...)
{
  /* temp buffer */
  char buf[2048];

  /* vararg parameters */
  va_list v;
  va_start(v, format);

  myvsprintf(buf, format, v);
  fputs(buf, stream);
}

#ifdef HOST_HAS_QWORD

#define LL_MAX		LL(9223372036854775807)
#define LL_MIN		(-LL_MAX - 1)
#define ULL_MAX		(ULL(9223372036854775807) * ULL(2) + 1)

/* convert a string to a signed result */
sqword_t
myatosq(char *nptr, char **endp, int base)
{
  char *s, *save;
  int negative, overflow;
  sqword_t cutoff, cutlim, i;
  unsigned char c;
  extern int errno;

  if (!nptr || !*nptr)
    panic("strtoll() passed a NULL string");

  s = nptr;

  /* skip white space */
  while (myisspace((int)(*s)))
    ++s;
  if (*s == '\0')
    goto noconv;

  if (base == 0)
    {
      if (s[0] == '0' && mytoupper(s[1]) == 'X')
	base = 16;
      else
	base = 10;
    }

  if (base <= 1 || base > 36)
    panic("bogus base: %d", base);

  /* check for a sign */
  if (*s == '-')
    {
      negative = 1;
      ++s;
    }
  else if (*s == '+')
    {
      negative = 0;
      ++s;
    }
  else
    negative = 0;

  if (base == 16 && s[0] == '0' && mytoupper(s[1]) == 'X')
    s += 2;

  /* save the pointer so we can check later if anything happened */
  save = s;

  cutoff = LL_MAX / (unsigned long int) base;
  cutlim = LL_MAX % (unsigned long int) base;

  overflow = 0;
  i = 0;
  for (c = *s; c != '\0'; c = *++s)
    {
      if (myisdigit (c))
        c -= '0';
      else if (myisalpha (c))
        c = mytoupper(c) - 'A' + 10;
      else
        break;
      if (c >= base)
        break;

      /* check for overflow */
      if (i > cutoff || (i == cutoff && c > cutlim))
        overflow = 1;
      else
        {
          i *= (unsigned long int) base;
          i += c;
        }
    }

  /* check if anything actually happened */
  if (s == save)
    goto noconv;

  /* store in ENDP the address of one character past the last character
     we converted */
  if (endp != NULL)
    *endp = (char *) s;

  if (overflow)
    {
      errno = ERANGE;
      return negative ? LL_MIN : LL_MAX;
    }
  else
    {
      errno = 0;

      /* return the result of the appropriate sign */
      return (negative ? -i : i);
    }

noconv:
  /* there was no number to convert */
  if (endp != NULL)
    *endp = (char *) nptr;
  return 0;
}

/* convert a string to a unsigned result */
qword_t
myatoq(char *nptr, char **endp, int base)
{
  char *s, *save;
  int overflow;
  qword_t cutoff, cutlim, i;
  unsigned char c;
  extern int errno;

  if (!nptr || !*nptr)
    panic("strtoll() passed a NULL string");

  s = nptr;

  /* skip white space */
  while (myisspace((int)(*s)))
    ++s;
  if (*s == '\0')
    goto noconv;

  if (base == 0)
    {
      if (s[0] == '0' && mytoupper(s[1]) == 'X')
	base = 16;
      else
	base = 10;
    }

  if (base <= 1 || base > 36)
    panic("bogus base: %d", base);

  if (base == 16 && s[0] == '0' && mytoupper(s[1]) == 'X')
    s += 2;

  /* save the pointer so we can check later if anything happened */
  save = s;

  cutoff = ULL_MAX / (unsigned long int) base;
  cutlim = ULL_MAX % (unsigned long int) base;

  overflow = 0;
  i = 0;
  for (c = *s; c != '\0'; c = *++s)
    {
      if (myisdigit (c))
        c -= '0';
      else if (myisalpha (c))
        c = mytoupper(c) - 'A' + 10;
      else
        break;
      if (c >= base)
        break;

      /* check for overflow */
      if (i > cutoff || (i == cutoff && c > cutlim))
        overflow = 1;
      else
        {
          i *= (unsigned long int) base;
          i += c;
        }
    }

  /* check if anything actually happened */
  if (s == save)
    goto noconv;

  /* store in ENDP the address of one character past the last character
     we converted */
  if (endp != NULL)
    *endp = (char *) s;

  if (overflow)
    {
      errno = ERANGE;
      return ULL_MAX;
    }
  else
    {
      errno = 0;

      /* return the result of the appropriate sign */
      return i;
    }

noconv:
  /* there was no number to convert */
  if (endp != NULL)
    *endp = (char *) nptr;
  return 0;
}

#endif /* HOST_HAS_QWORD */

#ifdef GZIP_PATH

static struct {
  char *type;
  char *ext;
  char *cmd;
} gzcmds[] = {
  /* type */	/* extension */		/* command */
  { "r",	".gz",			"%s -dc %s" },
  { "rb",	".gz",			"%s -dc %s" },
  { "r",	".Z",			"%s -dc %s" },
  { "rb",	".Z",			"%s -dc %s" },
  { "w",	".gz",			"%s > %s" },
  { "wb",	".gz",			"%s > %s" }
};

/* same semantics as fopen() except that filenames ending with a ".gz" or ".Z"
   will be automagically get compressed */
FILE *
gzopen(const char *fname, const char *type)
{
  int i;
  char *cmd = NULL;
  const char *ext;
  FILE *fd;
  char str[2048];

  /* get the extension */
  ext = mystrrchr(fname, '.');

  /* check if extension indicates compressed file */
  if (ext != NULL && *ext != '\0')
    {
      for (i=0; i < (int)N_ELT(gzcmds); i++)
	{
	  if (!strcmp(gzcmds[i].type, type) && !strcmp(gzcmds[i].ext, ext))
	    {
	      cmd = gzcmds[i].cmd;
	      break;
	    }
	}
    }

  if (!cmd)
    {
      /* open file */
      fd = fopen(fname, type);
    }
  else
    {
      /* open pipe to compressor/decompressor */
      sprintf(str, cmd, GZIP_PATH, fname);
      fd = popen(str, type);
    }

  return fd;
}

/* close compressed stream */
void
gzclose(FILE *fd)
{
  /* attempt pipe close, otherwise file close */
  if (pclose(fd) == -1)
    fclose(fd);
}

#else /* !GZIP_PATH */

FILE *
gzopen( const char *fname, const char *type)
{
  return fopen(fname, type);
}

void
gzclose(FILE *fd)
{
  fclose(fd);
}

#endif /* GZIP_PATH */

/* compute 32-bit CRC one byte at a time using the high-bit first (big-endian)
   bit ordering convention */
#define POLYNOMIAL 0x04c11db7L

static int crc_init = FALSE;
static unsigned long crc_table[256];

/* generate the table of CRC remainders for all possible bytes */
static void
crc_gentab(void)
{
  int i, j;
  dword_t crc_accum;

  for (i=0; i < 256; i++)
    {
      crc_accum = ((unsigned long)i << 24);
      for (j=0; j < 8; j++)
	{
	  if (crc_accum & 0x80000000L)
	    crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
	  else
	    crc_accum = (crc_accum << 1);
	}
      crc_table[i] = crc_accum;
    }
  return;
}

/* update the CRC on the data block one byte at a time */
dword_t
crc(dword_t crc_accum, dword_t data)
{
  int i, j;

  if (!crc_init)
    {
      crc_gentab();
      crc_init = TRUE;
    }

  for (j=0; j < (int)sizeof(dword_t); j++)
    {
      i = ((int)(crc_accum >> 24) ^ (data >> (j*8))) & 0xff;
      crc_accum = (crc_accum << 8) ^ crc_table[i];
    }
  return crc_accum;
}

/* swap a record, using machine/target dependent swap macros, REC points to
   the binary record to swap, the RECTYPE string specifies the type and swap
   operation applied to REC, as follows:

   'b' - byte no swap
   'w' - word no swap
   'd' - doubleword no swap
   'q' - quadword no swap
   'W' - half swap
   'D' - doubleword swap
   'Q' - quadword swap
*/
void
swaprec(void *rec, char *rectype)
{
  char *s, *p = (char*) rec;

  for (s=rectype; *s != '\0'; s++)
    {
      switch (*s)
	{
	case 'b':
	  /* byte - no swap */
	  p += sizeof(byte_t);
	  break;
	case 'w':
	  /* half - no swap */
	  p += sizeof(word_t);
	  break;
	case 'd':
	  /* word - no swap */
	  p += sizeof(dword_t);
	  break;
#ifdef HOST_HAS_QWORD
	case 'q':
	  /* quadword - no swap */
	  p += sizeof(qword_t);
	  break;
#endif /* HOST_HAS_QWORD */

	case 'W':
	  /* word - swap */
	  *((word_t *)p) = MD_SWAPW(*((word_t *)p));
	  p += sizeof(word_t);
	  break;
	case 'D':
	  /* doubleword - swap */
	  *((dword_t *)p) = MD_SWAPD(*((dword_t *)p));
	  p += sizeof(dword_t);
	  break;
#ifdef HOST_HAS_QWORD
	case 'Q':
	  /* quadword - swap */
	  *((qword_t *)p) = MD_SWAPQ(*((qword_t *)p));
	  p += sizeof(qword_t);
	  break;
#endif /* HOST_HAS_QWORD */

	default:
	  panic("bogus record field type: `%c'", *s);
	}
    }
}


/*****************************************************
   produces a random INTEGER in [imin, imax]
   ie, including the endpoints imin and imax
******************************************************/ 
/* random generating function */
double uniform_random()
{
  int ra = rand();
  return (((double) ra) / ((double) RAND_MAX));
}
/* produces a random Real in [rmin, rmax] */  
int uniform_random_irange( int imin, int imax)
{
  double u = uniform_random();
  int m = imin + (int)floor((double)(imax + 1 - imin)*u ) ;
  return  m ;
}   




/* The following are macros for basic memory operations.  If you have
   SSE support, these should run faster. */
void memzero(void * base, int bytes)
{
#ifdef USE_SSE_MOVE
  char * addr = (char*) base;

  asm ("xorps %%xmm0, %%xmm0"
       : : : "%xmm0");
  if((((unsigned long int)addr) & 0x0f) == 0) /* aligned */
    for(int i=0;i<bytes>>4;i++)
    {
      asm ("movaps %%xmm0,  (%0)\n\t"
           : : "r"(addr) : "memory");
      addr += 16;
    }
  else /* unaligned */
    for(int i=0;i<bytes>>4;i++)
    {
      asm ("movlps %%xmm0,  (%0)\n\t"
           "movlps %%xmm0, 8(%0)\n\t"
           : : "r"(addr) : "memory");
      addr += 16;
    }

  // remainder
  for(int i=0;i<(bytes&0x0f);i++)
  {
    *addr = 0;
    addr++;
  }
#else
  memset(base,0,bytes);
#endif
}

/* assumes aligned accesses */
void clear_page(void * base)
{
#ifdef USE_SSE_MOVE
  char * addr = (char*) base;

  asm ("xorps %%xmm0, %%xmm0"
       : : : "%xmm0");
  for(int i=0;i<MD_PAGE_SIZE/16/8;i++)
  {
    asm ("movntps %%xmm0,    (%0)\n\t"
         "movntps %%xmm0,  16(%0)\n\t"
         "movntps %%xmm0,  32(%0)\n\t"
         "movntps %%xmm0,  48(%0)\n\t"
         "movntps %%xmm0,  64(%0)\n\t"
         "movntps %%xmm0,  80(%0)\n\t"
         "movntps %%xmm0,  96(%0)\n\t"
         "movntps %%xmm0, 112(%0)\n\t"
         : : "r"(addr) : "memory");
    addr += 16*8;
  }
#else
  memset(base,0,MD_PAGE_SIZE);
#endif
}


/* swap contents of two memory buffers:
   SSE mode should be faster than equivalent memcpy
   version which would need to copy to a temporary
   memory buffer.  This version does the swap incrementally
   by making use of SSE/XMM registers for temp space.  Should
   reduce memory operations by about 33%. */
void memswap(void * p1, void * p2, size_t num_bytes)
{
  int i;
  char * addr1 = (char*) p1;
  char * addr2 = (char*) p2;
#ifdef USE_SSE_MOVE
  int iter = num_bytes >> 4; // div by 16
  int rem = num_bytes & 0x0f; // mod 16
  char tmp[16];

  /* both buffers 16-byte aligned */
  if(((((unsigned long int)p1)&0x0f) | (((unsigned long int)p2)&0x0f)) == 0)
  {
    for(i=0;i<iter;i++)
    {
      asm ("movaps (%0), %%xmm0\n\t" // load p1 into xmm0 and xmm1
           "movaps (%1), %%xmm1\n\t" // load p2 into xmm2 and xmm3
           "movaps %%xmm0, (%1)\n\t" // write p1 --> p2
           "movaps %%xmm1, (%0)\n\t" // write p2 --> p1
           : : "r"(addr1), "r"(addr2) : "memory","%xmm0","%xmm1");
      addr1 += 16;
      addr2 += 16;
    }
  }
  else /* unaligned */
  {
    for(i=0;i<iter;i++)
    {
      asm ("movlps (%0),  %%xmm0\n\t" // load p1 into xmm0 and xmm1
           "movlps 8(%0), %%xmm1\n\t"
           "movlps (%1),  %%xmm2\n\t" // load p2 into xmm2 and xmm3
           "movlps 8(%1), %%xmm3\n\t"
           "movlps %%xmm0,  (%1)\n\t" // write p1 --> p2
           "movlps %%xmm1, 8(%1)\n\t"
           "movlps %%xmm2,  (%0)\n\t" // write p2 --> p1
           "movlps %%xmm3, 8(%0)\n\t"
           : : "r"(addr1), "r"(addr2) : "memory","%xmm0","%xmm1","%xmm2","%xmm3");
      addr1 += 16;
      addr2 += 16;
    }
  }
#else
  int rem = num_bytes;
  char * tmp = (char*)alloca(num_bytes);
#endif

  // any remaining bytes
  memcpy(tmp,addr1,rem);
  memcpy(addr1,addr2,rem);
  memcpy(addr2,tmp,rem);
  /*
  for(i=0;i<rem;i++)
  {
    char tmp = *addr1;
    *addr1 = *addr2;
    *addr2 = tmp;
    addr1++;
    addr2++;
  }*/
}


#ifdef __cplusplus
}
#endif

