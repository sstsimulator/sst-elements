/*
** Copyright (c) 2011, IBM Corporation
** All rights reserved.
**
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
**
** Measure alltoall performance on an ever inresing number of nodes.
** We use my_alltoall so the same communication pattern is used here
** as well as in the pattern state machine.
** Test 1 measures the MPI_Alltoall time and serves as a comparison.
** Test 2 measures my_alltoall to compare againts test 1
*/

#include <stdio.h>
#include <stdlib.h>	/* For strtol(), exit() */
#include <stdint.h>	/* For uint32_t */
#include <unistd.h>	/* For getopt() */
#include <string.h>	/* For memset() */
#include <math.h>
#include <assert.h>
#include <mpi.h>
#include "Collectives/alltoall.h"
#include "util/stats.h"
extern "C" {
#include "stat_p.h"
#include "util.h"
}



/* Constants */
#define DEFAULT_PRECISION	(0.01)

#define MAX_TRIALS		(10000)
#define LARGE_ALLREDUCE_OPS	(1)
#define SMALL_ALLREDUCE_OPS	(20)
#define SMALL_LARGE_CUTOFF	(16)



/* Local functions */
static double do_one_Test1_trial(int num_ops, int num_doubles, MPI_Comm comm, bool check_data);
static double do_one_Test2_trial(int num_ops, int num_doubles, MPI_Comm comm, bool check_data);
static void experiment(int my_rank, int max_trials, int num_ops, int num_doubles, int nnodes,
    MPI_Comm comm, bool check_data, double req_precision,
    double (*do_one_trial)(int num_ops, int num_doubles, MPI_Comm comm, bool check_data));

static void usage(char *pname);



int
main(int argc, char *argv[])
{

int ch, error;
int my_rank, num_ranks;
int nnodes;
int num_doubles;
bool library;
bool stat_mode;
double req_precision;
int num_ops;
int max_trials;
bool check_data;
char processor_name[MPI_MAX_PROCESSOR_NAME];
int name_len;



    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    /* Set the defaults */
    opterr= 0;		/* Disable getopt error printing */
    error= false;
    num_doubles= 1;
    library= false;
    stat_mode= true;
    req_precision= DEFAULT_PRECISION;
    check_data= false;



    /* Check command line args */
    while ((ch= getopt(argc, argv, "a:cl:p:")) != EOF)   {
	switch (ch)   {
	    case 'a': // Algorithm
		if (strtol(optarg, (char **)NULL, 0) == 0)   {
		    library= false;
		} else if (strtol(optarg, (char **)NULL, 0) == 1)   {
		    library= true;
		} else   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Unknown algorithm. Must be 0 (my_alltoall) or 1 (MPI)!\n");
		    }
		    error= true;
		}
		break;

	    case 'c':
		check_data= true;
		break;

	    case 'l':
		num_doubles= strtol(optarg, (char **)NULL, 0);
		if (num_doubles < 1)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Message length in doubles (-l) must be > 0!\n");
		    }
		    error= true;
		}
		break;

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

    if (my_rank == 0)   {
	printf("# Alltoall benchmark\n");
	printf("# ------------------\n");
	disp_cmd_line(argc, argv);
	printf("#\n");
	if (stat_mode)   {
	    printf("# Requested precision is %.3f%%\n", req_precision * 100.0);
	} else   {
	    printf("# Statistics mode is off\n");
	}
	printf("# Message size is %d bytes\n", (int)(num_doubles * sizeof(double)));
	printf("# Algorithm used for Test 3: ");
	if (library)   {
	    printf("MPI_Alltoall\n");
	} else   {
	    printf("my_alltoall\n");
	}
	if (check_data)   {
	    printf("# Data checked after each alltoall operation\n");
	} else   {
	    printf("# Data will not be checked\n");
	}
	printf("#\n");
    }

    MPI_Get_processor_name(processor_name, &name_len);
    printf("# proc name for %4d/%-4d on %s\n", my_rank, num_ranks, processor_name);



    // For small alltoall, we do more than one to make sure we don't run into
    // issues with timer resolution.
    if (num_ranks < SMALL_LARGE_CUTOFF)   {
	num_ops= SMALL_ALLREDUCE_OPS;
    } else   {
	num_ops= LARGE_ALLREDUCE_OPS;
    }

    if (stat_mode)   {
	max_trials= MAX_TRIALS;
    } else   {
	max_trials= 1;
    }



    // Test 1
    if (my_rank == 0)   {
	printf("#  |||  Test 1: MPI_Alltoall()\n");
	printf("# nodes,        min,        mean,      median,         max,          sd,   precision, trials\n");
	printf("#");
    }
    experiment(my_rank, max_trials, num_ops, num_doubles, num_ranks, MPI_COMM_WORLD,
	    check_data, req_precision, &do_one_Test1_trial);



    // Test 2
    if (my_rank == 0)   {
	printf("#  |||  Test 2: my_alltoall()\n");
	printf("# nodes,        min,        mean,      median,         max,          sd,   precision, trials\n");
	printf("#");
    }
    experiment(my_rank, max_trials, num_ops, num_doubles, num_ranks, MPI_COMM_WORLD,
	    check_data, req_precision, &do_one_Test1_trial);


    // Test 3
    // Now we do this with increasing number of ranks, instead of all of them
    if (my_rank == 0)   {
	if (library)   {
	    printf("#  |||  Test 3: MPI_Alltoall()\n");
	} else   {
	    printf("#  |||  Test 3: my_alltoall()\n");
	}
	printf("# nodes,        min,        mean,      median,         max,          sd,   precision, trials\n");
    }

    nnodes= 1;
    while (nnodes <= num_ranks)   {
	// Adjust to the new node size
	MPI_Group world_group, new_group;
	MPI_Comm new_comm;
	int ranges[][3]={{0, nnodes - 1, 1}};

	MPI_Comm_group(MPI_COMM_WORLD, &world_group);
	MPI_Group_range_incl(world_group, 1, ranges, &new_group);
	MPI_Comm_create(MPI_COMM_WORLD, new_group, &new_comm);

	// For small alltoall, we do more than one to make sure we don't run into
	// issues with timer resolution.
	if (nnodes < SMALL_LARGE_CUTOFF)   {
	    num_ops= SMALL_ALLREDUCE_OPS;
	} else   {
	    num_ops= LARGE_ALLREDUCE_OPS;
	}

	if (my_rank < nnodes)   {
	    if (library)   {
		// Use the built-in MPI_Alltoall
		experiment(my_rank, max_trials, num_ops, num_doubles, nnodes, new_comm,
		    check_data, req_precision, &do_one_Test1_trial);
	    } else   {
		// Use my alltoall
		experiment(my_rank, max_trials, num_ops, num_doubles, nnodes, new_comm,
		    check_data, req_precision, &do_one_Test2_trial);
	    }
	}

	/* Don't measure every single node size... */
	if (nnodes == num_ranks)   {
	    break;
	}
	if (nnodes < 64)   {
	    nnodes++;
	} else if (nnodes < 128)   {
	    nnodes= nnodes + 8;
	    if (nnodes > num_ranks) nnodes= num_ranks;
	} else if (nnodes < 512)   {
	    nnodes= nnodes + 32;
	    if (nnodes > num_ranks) nnodes= num_ranks;
	} else if (nnodes < 2048)   {
	    nnodes= nnodes + 128;
	    if (nnodes > num_ranks) nnodes= num_ranks;
	} else   {
	    nnodes= nnodes + 512;
	    if (nnodes > num_ranks) nnodes= num_ranks;
	}
    }

    MPI_Finalize();

    return 0;

}  /* end of main() */



static double
do_one_Test1_trial(int num_ops, int num_doubles, MPI_Comm comm, bool check_data)
{

double t, t2;
int i, j;
double *sbuf;
double *rbuf;
int nranks;
int my_rank;


	MPI_Comm_size(comm, &nranks);
	MPI_Comm_rank(comm, &my_rank);

	// Allocate memory
	sbuf= (double *)malloc(sizeof(double) * num_doubles * nranks);
	rbuf= (double *)malloc(sizeof(double) * num_doubles * nranks);
	if ((sbuf == NULL) || (rbuf == NULL))   {
	    fprintf(stderr, "Out of memory!\n");
	    exit(-1);
	}
	for (j= 0; j < nranks; j++)   {
	    for (i= 0; i < num_doubles; i++)   {
		sbuf[j * num_doubles + i]= i + my_rank;
	    }
	}

	for (i= 0; i < num_doubles * nranks; i++)   {
	    rbuf[i]= -51.0;
	}

	/* Check */
	if (check_data)   {
	    MPI_Alltoall(sbuf, num_doubles, MPI_DOUBLE, rbuf, num_doubles, MPI_DOUBLE, comm);
	    for (j= 0; j < nranks; j++)   {
		for (i= 0; i < num_doubles; i++)   {
		    if (rbuf[j * num_doubles + i] != i + j)   {
			fprintf(stderr, "[%3d] MPI_Alltoall() failed at rbuf[j %d, i %d]\n", my_rank, j, i);
			exit(-1);
		    }
		}
	    }
	}

	/* Do a warm-up */
	MPI_Alltoall(sbuf, num_doubles, MPI_DOUBLE, rbuf, num_doubles, MPI_DOUBLE, comm);
	
	/* Start the timer */
	t= MPI_Wtime();
	for (i= 0; i < num_ops; i++)   {
	    MPI_Alltoall(sbuf, num_doubles, MPI_DOUBLE, rbuf, num_doubles, MPI_DOUBLE, comm);
	}
	t2= MPI_Wtime() - t;

	free(sbuf);
	free(rbuf);
	return t2 / num_ops;

}  // end of do_one_Test1_trial()



static double
do_one_Test2_trial(int num_ops, int num_doubles, MPI_Comm comm, bool check_data)
{

double t, t2;
int i, j;
double *sbuf;
double *rbuf;
int nranks;
int my_rank;


	MPI_Comm_rank(comm, &my_rank);
	MPI_Comm_size(comm, &nranks);

	// Allocate memory
	sbuf= (double *)malloc(sizeof(double) * num_doubles * nranks);
	rbuf= (double *)malloc(sizeof(double) * num_doubles * nranks);
	if ((sbuf == NULL) || (rbuf == NULL))   {
	    fprintf(stderr, "Out of memory!\n");
	    exit(-1);
	}
	for (j= 0; j < nranks; j++)   {
	    for (i= 0; i < num_doubles; i++)   {
		sbuf[j * num_doubles + i]= j * my_rank + i + my_rank;
	    }
	}

	for (i= 0; i < num_doubles * nranks; i++)   {
	    rbuf[i]= -51.0;
	}


	/* Check */
	if (check_data)   {
	    my_alltoall(sbuf, rbuf, num_doubles, nranks, my_rank);
	    for (j= 0; j < nranks; j++)   {
		for (i= 0; i < num_doubles; i++)   {
		    if (rbuf[j * num_doubles + i] != j * j + i + j)   {
			fprintf(stderr, "[%3d] %d ranks, my_alltoall() failed at rbuf[j %d, i %d]\n",
			    my_rank, nranks, j, i);
			if (my_rank == 4)   {
			for (int k= 0; k < nranks; k++)   {
			    fprintf(stderr, "[%3d]    [%2d][%2d] Expected %4.1f, got %4.1f\n", my_rank,
				k, i, (float)k * k + i + k, rbuf[k * num_doubles + i]);
			}
			}
			exit(-1);
		    }
		}
	    }
	}

	/* Do a warm-up */
	my_alltoall(sbuf, rbuf, num_doubles, nranks, my_rank);
	
	/* Start the timer */
	t= MPI_Wtime();
	for (i= 0; i < num_ops; i++)   {
	    my_alltoall(sbuf, rbuf, num_doubles, nranks, my_rank);
	}
	t2= MPI_Wtime() - t;

	free(sbuf);
	free(rbuf);
	return t2 / num_ops;

}  // end of do_one_Test2_trial()



static void
experiment(int my_rank, int max_trials, int num_ops, int num_doubles, int nnodes,
    MPI_Comm comm, bool check_data, double req_precision,
    double (*do_one_trial)(int num_ops, int num_doubles, MPI_Comm comm, bool check_data))
{

double tot;
double tot_squared;
double metric;
int cnt;
std::list <double>times;
double precision;
#if node0
int done;
#else
double total_time;
#endif


    times.clear();
    tot= 0.0;
    cnt= 0;
    tot_squared= 0.0;
    precision= 999.9;

    while (cnt < max_trials)   {
	cnt++;

	MPI_Barrier(comm);

	// Run one trial
	metric= do_one_trial(num_ops, num_doubles, comm, check_data);

#if !node0
	MPI_Allreduce(&metric, &total_time, 1, MPI_DOUBLE, MPI_SUM, comm);
	metric= total_time / nnodes;
#endif

	times.push_back(metric);

	tot= tot + metric;
	tot_squared= tot_squared + metric * metric;
	precision= stat_p(cnt, tot, tot_squared);

#if !node0
	// Need at least nine trials
	if ((cnt >= 9) && (precision < req_precision))   {
	    // Attained the required precisions. Done!
	    break;
	}
#else
	// Need at least nine trials
	if ((cnt >= 9) && (precision < req_precision))   {
	    // Attained the required precisions. Done!
	    done= 1;
	} else   {
	    done= 0;
	}
	// 0 tells everyone else
	MPI_Bcast(&done, 1, MPI_INT, 0, comm);
	if (done)   {
	    break;
	}
#endif
    }

    if (my_rank == 0)   {
	printf("%6d ", nnodes);
	print_stats(times, precision);
	printf(" %5d\n", cnt);
    }

}  // end of experiment()



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-l len] [-a type] [-p precision] [-c]\n", pname);
    fprintf(stderr, "    -a type        Select type of alltoall algorithm\n");
    fprintf(stderr, "                   0 = my_alltoall (default)\n");
    fprintf(stderr, "                   1 = MPI_Alltoall\n");

    fprintf(stderr, "    -l len      Size of alltoall operations in number of doubles. Default 1\n");
    fprintf(stderr, "    -p P        Set precision (confidence interval). Default %.3f\n",
	DEFAULT_PRECISION);
    fprintf(stderr, "                   A precision of 0.0 disables sampling\n");
    fprintf(stderr, "    -c             Check data\n");

}  /* end of usage() */
