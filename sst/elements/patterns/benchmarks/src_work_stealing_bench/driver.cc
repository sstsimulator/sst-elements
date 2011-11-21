/*
** Copyright (c) 2011, IBM Corporation
** All rights reserved.
**
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
**
** This is a wrapper for the work stealing benchmark. It performs a number of
** trials (each a work stealing benchmark run) and tries to achieve a set
** precision. It reports the actual error.
*/

#include <stdio.h>
#include <stdlib.h>	/* For strtol(), exit() */
#include <unistd.h>	/* For getopt() */
#include <string.h>	/* For strcmp() */
#include <mpi.h>
#include "util/stats.h"
extern "C" {
#include "benchmarks/stat_p.h"
#include "benchmarks/util.h"
}
#include "ws.h"


/* Constants */
#define DEFAULT_PRECISION	(0.05)
#define MAX_TRIALS		(20)



/* Globals */
int my_rank;
int num_ranks;



/* Local functions */
static void experiment(int max_trials, double req_precision);



int
main(int argc, char *argv[])
{

bool stat_mode;
double req_precision;
int max_trials;
int rc;



    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    /* Set the stat defaults */
    stat_mode= true;
    req_precision= DEFAULT_PRECISION;



    /* Check for my command line args */
    int p= 1;
    while (p < argc)   {
	if (strcmp("-p", argv[p]) == 0)   {
	    req_precision= strtod(argv[p + 1], (char **)NULL);
	    if (req_precision == 0.0)   {
		// Turn sampling off
		stat_mode= false;
	    }
	    // Eliminate them so getopt wont see them
	    argv[p]= (char *)"";
	    argv[p + 1]= (char *)"";
	}
	p++;
    }


    rc= ws_init(argc, argv);
    if (rc < 0)   {
	/* There was an error */
	MPI_Finalize();
	return rc;
    }

    if (rc > 0)   {
	/* No error, but prorgam wants to exit */
	MPI_Finalize();
	return 0;
    }

    if (stat_mode)   {
	max_trials= MAX_TRIALS;
    } else   {
	max_trials= 1;
    }

    if (my_rank == 0)   {
	if (stat_mode)   {
	    printf("# Requested precision is                                %6.3f%%\n",
		req_precision * 100.0);
	} else   {
	    printf("# Statistics mode is off. Will do a single trial.\n");
	}
	printf("#\n");
    }
    experiment(max_trials, req_precision);

    MPI_Finalize();
    return 0;

}  /* end of main() */



static void
experiment(int max_trials, double req_precision)
{

double tot;
double tot_squared;
double metric;
int cnt;
std::list <double>times;
double precision;
int done;


    times.clear();
    tot= 0.0;
    cnt= 0;
    tot_squared= 0.0;

    while (cnt < max_trials)   {
	cnt++;

	// Run one trial
	metric= ws_work();
	times.push_back(metric);

	tot= tot + metric;
	tot_squared= tot_squared + metric * metric;
	precision= stat_p(cnt, tot, tot_squared);

#if DEBUG
	if (my_rank == 0)   {
	    printf("Run %3d: time %.9f, precision so far %.3f%%\n", cnt, metric, precision);
	}
#endif

	// Need at least tree trials
	if ((cnt >= 3) && (precision < req_precision))   {
	    // Attained the required precisions. Done!
	    done= 1;
	} else   {
	    done= 0;
	}
	// 0 tells everyone else
	MPI_Bcast(&done, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if (done)   {
	    break;
	}
    }

    ws_print();
    if (my_rank == 0)   {
	printf("# nodes,        min,        mean,      median,         max,          sd,   precision, trials\n");
	printf("%6d ", num_ranks);
	print_stats(times, precision);
	printf(" %5d\n", cnt);
    }

}  // end of experiment()
