// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
** Some commonly used functions by the benchmarks
*/

#ifndef _UTIL_H_
#define _UTIL_H_

#ifndef TRUE
#define TRUE	(1)
#endif

#ifndef FALSE
#define FALSE	(0)
#endif

void disp_cmd_line(int argc, char **argv);

#define DEFAULT_CMD_LINE_ERR_CHECK \
    /* Command line error checking */ \
    case '?': \
	if (my_rank == 0)   { \
	    fprintf(stderr, "Unknown option \"%s\"\n", argv[optind - 1]); \
	} \
	error= TRUE; \
	break; \
    case ':': \
	if (my_rank == 0)   { \
		fprintf(stderr, "Missing option argument to \"%s\"\n", argv[optind - 1]); \
	} \
	error= TRUE; \
	break; \
    default: \
	error= TRUE; \
	break;

#endif  /* _UTIL_H_ */
