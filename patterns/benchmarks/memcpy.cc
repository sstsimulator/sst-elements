//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//
//

#include <stdio.h>
#include <stdlib.h>	/* For strtol(), exit() */
#include <unistd.h>	/* For getopt() */
#include <string.h>	/* For memset() */
#include <math.h>
#include <mpi.h>
#include "util/stats.h"
extern "C" {
#include "stat_p.h"
#include "util.h"
}


/* Constants */
#define DEFAULT_PRECISION	(0.05)

#define MAX_TRIALS		(20000)
#define LARGE_MEMCPY_OPS	(5)
#define MEDIUM_MEMCPY_OPS	(50)
#define SMALL_MEMCPY_OPS	(500)
#define SHORT_MEDIUM_CUTOFF	(int)( 2 * 1024)
#define MEDIUM_LONG_CUTOFF	(int)(16 * 1024)
#define MAX_NUM_BYTES		(int)(10485760)



/* Local functions */
static double do_one_trial(int num_ops, int bytes);
static void experiment(int my_rank, int max_trials, int num_ops, int bytes,
	double req_precision, double (*do_one_trial)(int num_ops, int bytes));

static void usage(char *pname);



int
main(int argc, char *argv[])
{

int ch;
bool error;
int my_rank, num_ranks;
bool stat_mode;
double req_precision;
int num_ops;
int max_trials;
int num_bytes;
int inc;
int end_num_bytes;


    MPI_Init(&argc, &argv);
 
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);



    /* Set the defaults */
    opterr= 0;		/* Disable getopt error printing */
    error= false;
    stat_mode= true;
    req_precision= DEFAULT_PRECISION;
    end_num_bytes= MAX_NUM_BYTES;
    inc= 8;



    /* Check command line args */
    while ((ch= getopt(argc, argv, "p:")) != EOF)   {
	switch (ch)   {
	    case 'p':
		req_precision= strtod(optarg, (char **)NULL);
		if (req_precision == 0.0)   {
		    // Turn sampling off
		    stat_mode= false;
		}
		break;

	    /* Command line error checking */
	    DEFAULT_CMD_LINE_ERR_CHECK
	}
    }
 
    if (error)   {
        if (my_rank == 0)   {
            usage(argv[0]);
        }
	MPI_Finalize();
        exit (-1);
    }



    if (stat_mode)   {
	max_trials= MAX_TRIALS;
    } else   {
	max_trials= 1;
    }

    if (my_rank == 0)   {
	printf("# Vector Add Benchmark\n");
	printf("# ------------=-------\n");
	disp_cmd_line(argc, argv);
	printf("#\n");
	if (stat_mode)   {
	    printf("# Requested precision is %.3f%%\n", req_precision * 100.0);
	} else   {
	    printf("# Statistics mode is off\n");
	}
	printf("# Num bytes     min,        mean,      median,         max,          "
	    "sd,   precision, trials,     per byte\n");
    }



    num_bytes= 8;
    while (num_bytes <= end_num_bytes)   {

	if (num_bytes < SHORT_MEDIUM_CUTOFF)   {
	    num_ops= SMALL_MEMCPY_OPS;
	} else if (num_bytes < MEDIUM_LONG_CUTOFF)   {
	    num_ops= MEDIUM_MEMCPY_OPS;
	} else   {
	    num_ops= LARGE_MEMCPY_OPS;
	}

	experiment(my_rank, max_trials, num_ops, num_bytes, req_precision, &do_one_trial);

	if (num_bytes >= (inc << 4))   {
	    inc= inc << 1;
	}
	num_bytes= num_bytes + inc;
    }

    MPI_Finalize();

    return 0;

}  /* end of main() */



static double
do_one_trial(int num_ops, int bytes)
{

double t, t2;
volatile double *sbuf;
volatile double *rbuf;


	// Allocate memory
	sbuf= (double *)malloc(bytes);
	rbuf= (double *)malloc(bytes);
	if ((sbuf == NULL) || (rbuf == NULL))   {
	    fprintf(stderr, "Out of memory!\n");
	    exit(-1);
	}
	memset((char *)sbuf, 0xAA, bytes);
	memset((char *)rbuf, 0x55, bytes);
	
	/* Start the timer */
	t= MPI_Wtime();
	for (int i= 0; i < num_ops; i++)   {
	    memcpy((void *)rbuf, (void *)sbuf, bytes);
	}
	t2= MPI_Wtime() - t;

	free((void *)sbuf);
	free((void *)rbuf);
	return t2 / num_ops;

}  // end of do_one_trial()



static void
experiment(int my_rank, int max_trials, int num_ops, int bytes,
    double req_precision, double (*do_one_trial)(int num_ops, int bytes))
{

double tot;
double tot_squared;
double metric;
int cnt;
std::list <double>times;
double precision;


    times.clear();
    tot= 0.0;
    cnt= 0;
    tot_squared= 0.0;

    while (cnt < max_trials)   {
	cnt++;

	// Run one trial
	metric= do_one_trial(num_ops, bytes);

	times.push_back(metric);

	tot= tot + metric;
	tot_squared= tot_squared + metric * metric;
	precision= stat_p(cnt, tot, tot_squared);

	// Need at least nine trials
	if ((cnt >= 9) && (precision < req_precision))   {
	    // Attained the required precisions. Done!
	    break;
	}
    }

    if (my_rank == 0)   {
	printf("%6d ", bytes);
	print_stats(times, precision);
	printf(" %5d     %15.12f\n", cnt, (tot / cnt) / bytes);
    }

}  // end of experiment()



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-p precision]\n", pname);
    fprintf(stderr, "    -p rpecision   Set precision (confidence interval). Default %.3f\n",
	DEFAULT_PRECISION);
    fprintf(stderr, "                   A precision of 0.0 disables sampling\n");

}  /* end of usage() */
