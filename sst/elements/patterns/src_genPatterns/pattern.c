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

#include <stdio.h>
#include <stdlib.h>	/* For strtol() */
#include <string.h>
#include "main.h"
#include "pattern.h"



/* For collectives */
typedef enum {TREE_BINARY= 0, TREE_DEEP} tree_type_t;


/* Local functions */
static void set_defaults(void);
static int error_check(void);
static char *str_tree_type(tree_type_t t);


#define MAX_LINE		(1024)
#define MAX_PATTERN_NAME	(256)

/* These are the pattern names we recognize */
#define ALLTOALL_NAME		"alltoall_pattern"
#define ALLREDUCE_NAME		"allreduce_pattern"
#define GHOST_NAME		"ghost_pattern"
#define PINGPONG_NAME		"pingpong_pattern"
#define MSGRATE_NAME		"msgrate_pattern"
#define FFT_NAME		"fft_pattern"

typedef enum {alltoall_pattern, allreduce_pattern, pingpong_pattern,
    ghost_pattern, msgrate_pattern, fft_pattern} pattern_t;


/* 
** Parameter variables. Locally global so I don't have to keep them
** passing back and forth (too many of them)
*/
static char _pattern_name[MAX_PATTERN_NAME];
pattern_t _pattern;


/* Allreduce parameters */
static int _num_ops;
static int _num_sets;
static int _num_doubles;
static int _start_nnodes;
static tree_type_t _tree_type;

/* Alltoall parameters */
    /* Reuse Allreduce parameters */
/* static int _num_ops; already declared */
/* static int _num_sets; already declared */
/* static int _num_doubles; already declared */
/* static int _start_nnodes; */
/* static tree_type_t _tree_type; already declared */

/* Ghost parameters */
static int _time_steps;		/* Run for num time steps. Default 1000 */
static int _x_elements;		/* Number of elements per rank in x dimension. Default 16384 */
static int _y_elements;		/* Number of elements per rank in y dimension. Default 16384 */
static int _z_elements;		/* Number of elements per rank in z dimension. Default 400 */
static int _loops;		/* Number of loops to compute. Adjusts compute time. Default 16 */
static int _reduce_steps;	/* Number of time steps between reduce operations. Default 20 */
static float _delay;		/* Delay compute step by delay % */
static int _imbalance;		/* Create compute imbalance. Requires delay > 0%. */
static int _verbose;
static int _time_per_flop;	/* This determines "compute" time */

/* Pingpong parameters */
static int _destination;
static int _num_msgs;
static int _end_len;
static int _len_inc;

/* Msgrate parameters */
/* static int _num_msgs; already declared */
static int _msg_len;
static int _rank_stride;
static int _start_rank;

/* FFT parameters */
static int _N;
static int _iter;
/* static tree_type_t _tree_type; already declared */
/* static int _time_per_flop; already declared */



static void
set_defaults(void)
{

    strcpy(_pattern_name, "");

    /* Alltoall defaults */
    /* Allreduce defaults */
    _num_ops= NO_DEFAULT;
    _num_sets= NO_DEFAULT;
    _num_doubles= 1;
    _tree_type= TREE_DEEP;
    _start_nnodes= 0;

    /* Ghost defaults */
    _time_steps= 1000;
    _x_elements= 400;
    _y_elements= 400;
    _z_elements= 400;
    _loops= 16;
    _reduce_steps= 20;
    _delay= 0;
    _imbalance= 0;
    _verbose= 0;
    _time_per_flop= 10;

    /* Pingpong defaults */
    _destination= OPTIONAL;
    _num_msgs= NO_DEFAULT;
    _end_len= OPTIONAL;
    _len_inc= OPTIONAL;

    /* Msgrate defaults */
    /* _num_msgs= NO_DEFAULT; already set */
    _msg_len= NO_DEFAULT;
    _rank_stride= NO_DEFAULT;
    _start_rank= NO_DEFAULT;

    /* FFT defaults */
    _N= NO_DEFAULT;
    _iter= OPTIONAL;
    /* _tree_type= TREE_DEEP; already set */

}  /* end of set_defaults() */



#define PARAM_CHECK(p, x, c, v) \
    if (_##x c v)   { \
        error= TRUE; \
	fprintf(stderr, "%s parameter \"%s\" must be specified in input file\n", p, #x); \
    }

static int
error_check(void)
{

int error;


    error= FALSE;
    if (strcmp(_pattern_name, "") == 0)   {
	error= TRUE;
	fprintf(stderr, "Pattern name must be specified in input file\n");
    }

    switch (_pattern)   {
	case alltoall_pattern:
	    PARAM_CHECK("Alltoall", num_ops, <, 0);
	    PARAM_CHECK("Alltoall", num_sets, <, 0);
	    PARAM_CHECK("Alltoall", num_doubles, <, 0);
	    break;

	case allreduce_pattern:
	    PARAM_CHECK("Allreduce", num_ops, <, 0);
	    PARAM_CHECK("Allreduce", num_sets, <, 0);
	    PARAM_CHECK("Allreduce", num_doubles, <, 0);
	    break;

	case ghost_pattern:
	    PARAM_CHECK("Ghost", time_steps, <, 0);
	    PARAM_CHECK("Ghost", x_elements, <, 0);
	    PARAM_CHECK("Ghost", y_elements, <, 0);
	    PARAM_CHECK("Ghost", z_elements, <, 0);
	    PARAM_CHECK("Ghost", loops, <, 0);
	    PARAM_CHECK("Ghost", reduce_steps, <, 0);
	    PARAM_CHECK("Ghost", delay, <, 0);
	    PARAM_CHECK("Ghost", imbalance, <, 0);
	    PARAM_CHECK("Ghost", verbose, <, 0);
	    PARAM_CHECK("Ghost", time_per_flop, <, 0);
	    break;

	case pingpong_pattern:
	    /* Don't check optional parameter "destination" */
	    PARAM_CHECK("Pingpong", num_msgs, <, 0);
	    /* Don't check optional parameter "end_len" */
	    /* Don't check optional parameter "len_inc" */
	    break;

	case msgrate_pattern:
	    PARAM_CHECK("Msgrate", num_msgs, <, 0);
	    PARAM_CHECK("Msgrate", msg_len, <, 0);
	    PARAM_CHECK("Msgrate", rank_stride, <, 0);
	    PARAM_CHECK("Msgrate", start_rank, <, 0);
	    break;

	case fft_pattern:
	    PARAM_CHECK("FFT", N, <, 0);
	    PARAM_CHECK("FFT", time_per_flop, <, 0);
	    /* Don't check optional parameter "iter" */
	    break;
    }

    if (error)   {
	return FALSE;
    }

    /* No errors found */
    return TRUE;

}  /* end of error_check() */



void
disp_pattern_params(void)
{

    printf("# *** Pattern name \"%s\"\n", pattern_name());
    switch (_pattern)   {
	case alltoall_pattern:
	    printf("# ***     num_sets =     %d\n", _num_sets);
	    printf("# ***     num_ops =      %d\n", _num_ops);
	    printf("# ***     num_doubles =  %d\n", _num_doubles);
	    break;

	case allreduce_pattern:
	    printf("# ***     num_sets =     %d\n", _num_sets);
	    printf("# ***     num_ops =      %d\n", _num_ops);
	    printf("# ***     num_doubles =  %d\n", _num_doubles);
	    printf("# ***     tree_type =    %s\n", str_tree_type(_tree_type));
	    break;

	case ghost_pattern:
	    printf("# ***     time_steps =      %d\n", _time_steps);
	    printf("# ***     x_elements =      %d\n", _x_elements);
	    printf("# ***     y_elements =      %d\n", _y_elements);
	    printf("# ***     z_elements =      %d\n", _z_elements);
	    printf("# ***     loops =           %d\n", _loops);
	    printf("# ***     reduce_steps =    %d\n", _reduce_steps);
	    printf("# ***     delay =           %.2f%%\n", _delay);
	    printf("# ***     imbalance =       %d\n", _imbalance);
	    printf("# ***     verbose =         %d\n", _verbose);
	    printf("# ***     time_per_flop =   %d ns\n", _time_per_flop);
	    break;

	case pingpong_pattern:
	    if (_destination == OPTIONAL)   {
		printf("# ***     destination =  default\n");
	    } else   {
		printf("# ***     destination =  %d\n", _destination);
	    }
	    printf("# ***     num_msgs =     %d\n", _num_msgs);
	    if (_end_len == OPTIONAL)   {
		printf("# ***     end_len =      default\n");
	    } else   {
		printf("# ***     end_len =      %d\n", _end_len);
	    }
	    if (_len_inc == OPTIONAL)   {
		printf("# ***     len_inc =      default\n");
	    } else   {
		printf("# ***     len_inc =      %d\n", _len_inc);
	    }
	    break;

	case msgrate_pattern:
	    printf("# ***     num_msgs =     %d\n", _num_msgs);
	    printf("# ***     msg_len =      %d\n", _msg_len);
	    printf("# ***     start_rank =   %d\n", _start_rank);
	    if (_rank_stride == 0)   {
		printf("# ***     rank_stride =  none. Send to a single destination\n");
	    } else   {
		printf("# ***     rank_stride =  %d\n", _rank_stride);
	    }
	    break;

	case fft_pattern:
	    printf("# ***     N =               %d\n", _N);
	    if (_iter == OPTIONAL)   {
		printf("# ***     iter =            default\n");
	    } else   {
		printf("# ***     iter =            %d\n", _iter);
	    }
	    printf("# ***     tree_type =       %s\n", str_tree_type(_tree_type));
	    printf("# ***     time_per_flop =   %d ns\n", _time_per_flop);
	    break;
    }

}  /* end of disp_pattern_params() */



int
read_pattern_file(FILE *fp_pattern, int verbose)
{

int error;
char line[MAX_LINE];
char *pos;
char key[MAX_LINE];
char value1[MAX_LINE];
int rc;


    if (fp_pattern == NULL)   {
	return FALSE;
    }

    /* Defaults */
    error= FALSE;
    set_defaults();


    /* Process the input file */
    while (fgets(line, MAX_LINE, fp_pattern) != NULL)   {
	/* Get rid of comments */
	pos= strchr(line, '#');
	if (pos)   {
	    *pos= '\0';
	}

	/* Now scan it */
	rc= sscanf(line, "%s = %s", key, value1);
	if (rc != 2)   {
	    rc= sscanf(line, "param %s = %s", key, value1);
	    if (rc != 2)   {
		continue;
	    } else   {

		/* Process parameter entries */
		if (verbose > 1)   {
		    printf("Debug: Found parameter %s = %s\n", key, value1);
		}


		/* Alltoall parameters */
		/* Allreduce parameters */
		if (strcmp("num_sets", key) == 0)   {
		    _num_sets= strtol(value1, (char **)NULL, 0);

		} else if (strcmp("num_ops", key) == 0)   {
		    _num_ops= strtol(value1, (char **)NULL, 0);

		} else if (strcmp("num_doubles", key) == 0)   {
		    _num_doubles= strtol(value1, (char **)NULL, 0);

		} else if (strcmp("tree_type", key) == 0)   {
		    _tree_type= strtol(value1, (char **)NULL, 0);

		} else if (strcmp("start_nnodes", key) == 0)   {
		    _start_nnodes= strtol(value1, (char **)NULL, 0);


		/* Ghost parameters */
		} else if (strcmp("time_steps", key) == 0)   {
		    _time_steps= strtol(value1, (char **)NULL, 0);
		} else if (strcmp("x_elements", key) == 0)   {
		    _x_elements= strtol(value1, (char **)NULL, 0);
		} else if (strcmp("y_elements", key) == 0)   {
		    _y_elements= strtol(value1, (char **)NULL, 0);
		} else if (strcmp("z_elements", key) == 0)   {
		    _z_elements= strtol(value1, (char **)NULL, 0);
		} else if (strcmp("loops", key) == 0)   {
		    _loops= strtol(value1, (char **)NULL, 0);
		} else if (strcmp("reduce_steps", key) == 0)   {
		    _reduce_steps= strtol(value1, (char **)NULL, 0);
		} else if (strcmp("delay", key) == 0)   {
		    _delay= strtod(value1, (char **)NULL);
		} else if (strcmp("imbalance", key) == 0)   {
		    _imbalance= strtol(value1, (char **)NULL, 0);
		} else if (strcmp("verbose", key) == 0)   {
		    _verbose= strtol(value1, (char **)NULL, 0);
		} else if (strcmp("time_per_flop", key) == 0)   {
		    _time_per_flop= strtol(value1, (char **)NULL, 0);

		/* Pingpong parameters */
		} else if (strcmp("destination", key) == 0)   {
		    _destination= strtol(value1, (char **)NULL, 0);

		} else if (strcmp("num_msgs", key) == 0)   {
		    _num_msgs= strtol(value1, (char **)NULL, 0);

		} else if (strcmp("end_len", key) == 0)   {
		    _end_len= strtol(value1, (char **)NULL, 0);

		} else if (strcmp("len_inc", key) == 0)   {
		    _len_inc= strtol(value1, (char **)NULL, 0);


		/* Msgrate parameters */
		} else if (strcmp("msg_len", key) == 0)   {
		    _msg_len= strtol(value1, (char **)NULL, 0);
		} else if (strcmp("start_rank", key) == 0)   {
		    _start_rank= strtol(value1, (char **)NULL, 0);
		} else if (strcmp("rank_stride", key) == 0)   {
		    _rank_stride= strtol(value1, (char **)NULL, 0);

		/* FFT parameters */
		} else if (strcmp("N", key) == 0)   {
		    _N= strtol(value1, (char **)NULL, 0);

		} else if (strcmp("iter", key) == 0)   {
		    _iter= strtol(value1, (char **)NULL, 0);

		} else   {
		    fprintf(stderr, "Unknown parameter \"%s\" in pattern file!\n", key);
		    error= TRUE;
		}
	    }
	} else   {

	    /* Process name entries */
	    if (verbose > 1)   {
		printf("Debug: Found %s = %s\n", key, value1);
	    }

	    if (strcmp("name", key) == 0)   {
		strncpy(_pattern_name, value1, MAX_PATTERN_NAME);
		if (strcmp(_pattern_name, ALLTOALL_NAME) == 0)   {
		    _pattern= alltoall_pattern;
		} else if (strcmp(_pattern_name, ALLREDUCE_NAME) == 0)   {
		    _pattern= allreduce_pattern;
		} else if (strcmp(_pattern_name, GHOST_NAME) == 0)   {
		    _pattern= ghost_pattern;
		} else if (strcmp(_pattern_name, PINGPONG_NAME) == 0)   {
		    _pattern= pingpong_pattern;
		} else if (strcmp(_pattern_name, MSGRATE_NAME) == 0)   {
		    _pattern= msgrate_pattern;
		} else if (strcmp(_pattern_name, FFT_NAME) == 0)   {
		    _pattern= fft_pattern;
		} else   {
		    fprintf(stderr, "Unknown pattern name \"%s\" in pattern file!\n", _pattern_name);
		    error= TRUE;
		}

	    } else   {
		fprintf(stderr, "Unknown entry \"%s\" in pattern file!\n", key);
		error= TRUE;
	    }
	}
    }

    if (error)   {
	return FALSE;
    }

    if (error_check() == FALSE)   {
	return FALSE;
    }

    /* Seems OK */
    return TRUE;

}  /* end of read_pattern_file() */



char *
pattern_name(void)
{
    return _pattern_name;
}  /* end of pattern_name() */



#define PRINT_PARAM(f, p) \
    fprintf(f, "\t\t<%s> %d </%s>\n", #p, _##p, #p);

void
pattern_params(FILE *out)
{

    switch (_pattern)   {
	case alltoall_pattern:
	case allreduce_pattern:
	    PRINT_PARAM(out, num_sets);
	    PRINT_PARAM(out, num_ops);
	    PRINT_PARAM(out, num_doubles);
	    PRINT_PARAM(out, start_nnodes);
	    fprintf(out, "\t\t<tree_type> %s </tree_type>\n", str_tree_type(_tree_type));
	    break;

	case ghost_pattern:
	    PRINT_PARAM(out, time_steps);
	    PRINT_PARAM(out, x_elements);
	    PRINT_PARAM(out, y_elements);
	    PRINT_PARAM(out, z_elements);
	    PRINT_PARAM(out, loops);
	    PRINT_PARAM(out, reduce_steps);
	    fprintf(out, "\t\t<delay> %.2f </delay>\n", _delay);
	    PRINT_PARAM(out, imbalance);
	    PRINT_PARAM(out, verbose);
	    PRINT_PARAM(out, time_per_flop);
	    break;

	case pingpong_pattern:
	    if (_destination != OPTIONAL)   {
		PRINT_PARAM(out, destination);
	    }
	    PRINT_PARAM(out, num_msgs);
	    if (_end_len != OPTIONAL)   {
		PRINT_PARAM(out, end_len);
	    }
	    if (_len_inc != OPTIONAL)   {
		PRINT_PARAM(out, len_inc);
	    }
	    break;

	case msgrate_pattern:
	    PRINT_PARAM(out, num_msgs);
	    PRINT_PARAM(out, msg_len);
	    PRINT_PARAM(out, start_rank);
	    PRINT_PARAM(out, rank_stride);
	    break;

	case fft_pattern:
	    PRINT_PARAM(out, N);
	    if (_iter != OPTIONAL)   {
		PRINT_PARAM(out, iter);
	    }
	    PRINT_PARAM(out, time_per_flop);
	    fprintf(out, "\t\t<tree_type> %s </tree_type>\n", str_tree_type(_tree_type));
	    break;
    }

}  /* end of pattern_params() */



static char *
str_tree_type(tree_type_t t)
{

    switch (t)   {
	case TREE_DEEP:
	    return "deep";
	    break;
	case TREE_BINARY:
	    return "binary";
	    break;
	default:
	    return "unknown";
    }

}  /* end of str_tree_type() */
