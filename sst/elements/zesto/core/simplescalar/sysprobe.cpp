/* sysprobe.c - host endian probe implementation */
/* 
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
#ifndef _MSC_VER
#include <unistd.h>
#else /* _MSC_VER */
#define access	_access
#define X_OK	4
#endif

#include "host.h"
#include "misc.h"

char *gzip_paths[] =
{
  (char*)"/bin/gzip",
  (char*)"/usr/bin/gzip",
  (char*)"/usr/local/bin/gzip",
  (char*)"/usr/gnu/bin/gzip",
  (char*)"/usr/local/gnu/bin",
  NULL
};

#define HOST_ONLY
#include "endian.c"

#define CAT(a,b)	a/**/b

#define MSB		0x80000000
int
fast_SRL(void)
{
  dword_t ui;

  if (sizeof(ui) != 4)
    {
      /* fundamental size assumption broken - emulate SRL */
      return FALSE;
    }

  ui = (dword_t)MSB;
  if (((ui >> 1) & MSB) != 0)
    {
      /* unsigned int does SRA - emulate SRL */
      return FALSE;
    }
  else
    {
      /* unsigned int does SRL - use fast native SRL */
      return TRUE;
    }
}

int
fast_SRA(void)
{
  sdword_t si;

  if (sizeof(si) != 4)
    {
      /* fundamental size assumption broken - emulate SRA */
      return FALSE;
    }

  si = (sdword_t)MSB;
  if ((si >> 1) & MSB)
    {
      /* signed int does SRA - use fast native SRA */
      return TRUE;
    }
  else
    {
      /* singned int does SRL - emulate SRA */
      return FALSE;
    }
}

int
main(int argc, char **argv)
{
  int little_bytes = 0, little_words = 0;

  if (argc == 2 && !strcmp(argv[1], "-s"))
    {
      switch (endian_host_byte_order())
	{
	case endian_big:
	  fprintf(stdout, "big\n");
	  break;
	case endian_little:
	  fprintf(stdout, "little\n");
	  break;
	case endian_unknown:
	  fprintf(stderr, "\nerror: cannot determine byte order!\n");
	  exit(1);
	default:
	  abort();
	}
    }
  else if (argc == 2 && !strcmp(argv[1], "-libs"))
    {
#ifdef BFD_LOADER
      fprintf(stdout, "-lbfd -liberty ");
#endif /* BFD_LOADER */

#ifdef linux
      /* nada... */
#elif defined(__USLC__) || (defined(__svr4__) && defined(__i386__) && defined(__unix__))
      fprintf(stdout, "-L/usr/ucblib -lucb ");
#else
      /* nada */
#endif
      fprintf(stdout, " \n");
    }
  else if (argc == 1 || (argc == 2 && !strcmp(argv[1], "-flags")))
    {
      switch (endian_host_byte_order())
	{
	case endian_big:
	  fprintf(stdout, "-DBYTES_BIG_ENDIAN ");
	  break;
	case endian_little:
	  fprintf(stdout, "-DBYTES_LITTLE_ENDIAN ");
	  little_bytes = 1;
	  break;
	case endian_unknown:
	  fprintf(stderr, "\nerror: cannot determine byte order!\n");
	  exit(1);
	default:
	  abort();
	}

      switch (endian_host_word_order())
	{
	case endian_big:
	  fprintf(stdout, "-DWORDS_BIG_ENDIAN ");
	  break;
	case endian_little:
	  fprintf(stdout, "-DWORDS_LITTLE_ENDIAN ");
	  little_words = 1;
	  break;
	case endian_unknown:
	  fprintf(stderr, "\nerror: cannot determine word order!\n");
	  exit(1);
	default:
	  abort();
	}

#ifdef _AIX
	fprintf(stdout, "-D_ALL_SOURCE ");
#endif /* _AIX */

#if (defined(hpux) || defined(__hpux)) && !defined(__GNUC__)
	fprintf(stdout, "-D_INCLUDE_HPUX_SOURCE -D_INCLUDE_XOPEN_SOURCE -D_INCLUDE_AES_SOURCE ");
#endif /* hpux */

#ifndef __GNUC__
      /* probe compiler approach needed to concatenate symbols in CPP,
	 new style concatenation is always used with GNU GCC */
      {
	int i = 5, j;

	j = CAT(-,-i);

	if (j == 4)
	  {
	    /* old style symbol concatenation worked */
	    fprintf(stdout, "-DOLD_SYMCAT ");
	  }
	else if (j == 5)
	  {
	    /* old style symbol concatenation does not work, assume that
	       new style symbol concatenation works */
	    ;
	  }
	else
	  {
	    /* huh!?!?! */
	    fprintf(stderr, "\nerror: cannot grok symbol concat method!\n");
	    exit(1);
	  }
      }
#endif /* __GNUC__ */

#ifndef SLOW_SHIFTS
      /* probe host shift capabilities */
      if (fast_SRL())
	fprintf(stdout, "-DFAST_SRL ");
      if (fast_SRA())
	fprintf(stdout, "-DFAST_SRA ");
#endif /* !SLOW_SHIFTS */

      /* locate GZIP */
#ifndef GZIP_PATH
      {
	int i;

	for (i=0; gzip_paths[i] != NULL; i++)
	  {
	    if (access(gzip_paths[i], X_OK) == 0)
	      {
		fprintf(stdout, "-DGZIP_PATH=\"%s\" ", gzip_paths[i]);
		break;
	      }
	  }
      }
#endif /* !GZIP_PATH */

    }
  else if (argc == 2 && !strcmp(argv[1], "-t"))
    {
      fprintf(stdout, "sizeof(int) = %d\n", sizeof(int));
      fprintf(stdout, "sizeof(long) = %d\n", sizeof(long));
    }


  /* check for different byte/word endian-ness */
  if (little_bytes != little_words)
    {
      fprintf(stderr,
	      "\nerror: opposite byte/word endian currently not supported!\n");
      exit(1);
    }
  exit(0);
  return 0;
}

