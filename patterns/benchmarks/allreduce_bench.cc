/*
** Copyright (c) 2011, IBM Corporation
** All rights reserved.
**
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
**
** Measure allreduce performance on an ever inresing number of nodes.
** We use my_allreduce so the same communication pattern is used here
** as well as in the pattern state machine.
** Test 1 measures the MPI_Allreduce time and serves as a comparison.
** Test 2 measures my_allreduce to compare against test 1
**
** The number of trials for each experiment (node size) is not fixed.
** Enough trials are done until the desired precision is achieved.
*/

#include <stdio.h>
#include <stdlib.h>	/* For strtol(), exit() */
#include <unistd.h>	/* For getopt() */
#include <string.h>	/* For memset() */
#include <math.h>
#include <mpi.h>
#include "collective_patterns/collective_topology.h"
#include "Collectives/allreduce.h"
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
static double do_one_Test1_trial(int num_ops, int msg_len, MPI_Comm comm, Collective_topology *ctopo);
static double do_one_Test2_trial(int num_ops, int msg_len, MPI_Comm comm, Collective_topology *ctopo);
static void experiment(int my_rank, int max_trials, int num_ops, int msg_len, int nnodes,
	MPI_Comm comm, Collective_topology *ctopo, double req_precision,
	double (*do_one_trial)(int num_ops, int msg_len, MPI_Comm comm, Collective_topology *ctopo));

static void usage(char *pname);



int
main(int argc, char *argv[])
{

int ch;
bool error;
int my_rank, num_ranks;
Collective_topology *ctopo;
int nnodes;
int msg_len;
bool library;
tree_type_t tree;
bool stat_mode;
double req_precision;
int num_ops;
int max_trials;



    MPI_Init(&argc, &argv);
 
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    if (num_ranks < 2)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Need to run on at least two ranks; more would be better\n");
	}
	MPI_Finalize();
	exit(-1);
    }


    /* Set the defaults */
    opterr= 0;		/* Disable getopt error printing */
    error= false;
    msg_len= 1; // 1 sizeof(double)
    library= false;
    tree= TREE_DEEP;
    stat_mode= true;
    req_precision= DEFAULT_PRECISION;



    /* Check command line args */
    while ((ch= getopt(argc, argv, "a:l:p:")) != EOF)   {
	switch (ch)   {
	    case 'a': // Algorithm
		if (strtol(optarg, (char **)NULL, 0) == 0)   {
		    tree= TREE_DEEP;
		    library= false;
		} else if (strtol(optarg, (char **)NULL, 0) == 1)   {
		    tree= TREE_BINARY;
		    library= false;
		} else if (strtol(optarg, (char **)NULL, 0) == 2)   {
		    library= true;
		} else   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Unknown algorithm. Must be 0 (deep), 1 (binary), or 2 (MPI)!\n");
		    }
		    error= true;
		}
		break;

	    case 'l':
		msg_len= strtol(optarg, (char **)NULL, 0);
		if (msg_len < 1)   {
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
	printf("# Allreduce benchmark\n");
	printf("# -------------------\n");
	disp_cmd_line(argc, argv);
	printf("#\n");
	if (stat_mode)   {
	    printf("# Requested precision is %.3f%%\n", req_precision * 100.0);
	} else   {
	    printf("# Statistics mode is off\n");
	}
	printf("# Message size is %d bytes\n", (int)(msg_len * sizeof(double)));
	printf("# Algorithm used for Test 3: ");
	if (library)   {
	    printf("MPI_Allreduce\n ");
	} else   {
	    if (tree == TREE_BINARY)   {
		printf("binary\n ");
	    } else if (tree == TREE_DEEP)   {
		printf("deep\n");
	    } else   {
		printf("unknown\n ");
	    }
	}
    }



    // For small allreduces, we do more than one to make sure we don't run into
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
	printf("#  |||  Test 1: MPI_Allreduce()\n");
	printf("# nodes,        min,        mean,      median,         max,          sd,   precision, trials\n");
	printf("#");
    }
    experiment(my_rank, max_trials, num_ops, msg_len, num_ranks, MPI_COMM_WORLD,
	    NULL, req_precision, &do_one_Test1_trial);



    // Test 2
    if (my_rank == 0)   {
	if (tree == TREE_BINARY)   {
	    printf("#  |||  Test 2: my_allreduce() using binary tree\n");
	} else if (tree == TREE_DEEP)   {
	    printf("#  |||  Test 2: my_allreduce() using deep tree\n");
	} else   {
	    printf("#  |||  Test 2: my_allreduce() using unknown tree type\n");
	}
	printf("# nodes,        min,        mean,      median,         max,          sd,   precision, trials\n");
	printf("#");
    }
    ctopo= new Collective_topology(my_rank, num_ranks, tree);
    experiment(my_rank, max_trials, num_ops, msg_len, num_ranks, MPI_COMM_WORLD,
	    ctopo, req_precision, &do_one_Test2_trial);



    // Test 3
    // Now we do this with increasing number of ranks, instead of all of them
    if (my_rank == 0)   {
	printf("#  |||  Test 3: ");
	if (library)   {
	    printf("#  |||  Test 3: Using MPI library MPI_allreduce()\n");
	} else   {
	    if (tree == TREE_BINARY)   {
		printf("#  |||  Test 3: my_allreduce() using binary tree\n");
	    } else if (tree == TREE_DEEP)   {
		printf("#  |||  Test 3: my_allreduce() using deep tree\n");
	    } else   {
		printf("#  |||  Test 3: my_allreduce() using unknown tree type\n");
	    }
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

	delete ctopo;
	ctopo= new Collective_topology(my_rank, nnodes, tree);

	// For small allreduces, we do more than one to make sure we don't run into
	// issues with timer resolution.
	if (nnodes < 16)   {
	    num_ops= SMALL_ALLREDUCE_OPS;
	} else   {
	    num_ops= LARGE_ALLREDUCE_OPS;
	}

	if (my_rank < nnodes)   {
	    if (library)   {
		// Use the built-in MPI_Allreduce
		experiment(my_rank, max_trials, num_ops, msg_len, nnodes, new_comm,
		    ctopo, req_precision, &do_one_Test1_trial);
	    } else   {
		// Use my allreduce
		experiment(my_rank, max_trials, num_ops, msg_len, nnodes, new_comm,
		    ctopo, req_precision, &do_one_Test2_trial);
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
do_one_Test1_trial(int num_ops, int msg_len, MPI_Comm comm,
    Collective_topology *ctopo __attribute__ ((unused)))
{

double t, t2;
int i;
double *sbuf;
double *rbuf;


	// Allocate memory
	sbuf= (double *)malloc(sizeof(double) * msg_len);
	rbuf= (double *)malloc(sizeof(double) * msg_len);
	if ((sbuf == NULL) || (rbuf == NULL))   {
	    fprintf(stderr, "Out of memory!\n");
	    exit(-1);
	}

	for (i= 0; i < msg_len; i++)   {
	    sbuf[i]= 1.0;
	    rbuf[i]= -51.0;
	}

	
	/* Do a warm-up */
	MPI_Allreduce(sbuf, rbuf, msg_len, MPI_DOUBLE, MPI_SUM, comm);

	/* Start the timer */
	t= MPI_Wtime();
	for (i= 0; i < num_ops; i++)   {
	    MPI_Allreduce(sbuf, rbuf, msg_len, MPI_DOUBLE, MPI_SUM, comm);
	}
	t2= MPI_Wtime() - t;

	free(sbuf);
	free(rbuf);
	return t2 / num_ops;

}  // end of do_one_Test1_trial()



static double
do_one_Test2_trial(int num_ops, int msg_len, MPI_Comm comm __attribute__ ((unused)),
    Collective_topology *ctopo)
{

double t, t2;
int i;
double *sbuf;
double *rbuf;


	// Allocate memory
	sbuf= (double *)malloc(sizeof(double) * msg_len);
	rbuf= (double *)malloc(sizeof(double) * msg_len);
	if ((sbuf == NULL) || (rbuf == NULL))   {
	    fprintf(stderr, "Out of memory!\n");
	    exit(-1);
	}

	for (i= 0; i < msg_len; i++)   {
	    sbuf[i]= 1.0;
	    rbuf[i]= -51.0;
	}

	/* Do a warm-up */
	my_allreduce(sbuf, rbuf, msg_len, ctopo);

	/* Start the timer */
	t= MPI_Wtime();
	for (i= 0; i < num_ops; i++)   {
	    my_allreduce(sbuf, rbuf, msg_len, ctopo);
	}
	t2= MPI_Wtime() - t;

	free(sbuf);
	free(rbuf);
	return t2 / num_ops;

}  // end of do_one_Test2_trial()



static void
experiment(int my_rank, int max_trials, int num_ops, int msg_len, int nnodes,
    MPI_Comm comm, Collective_topology *ctopo, double req_precision,
    double (*do_one_trial)(int num_ops, int msg_len, MPI_Comm comm, Collective_topology *ctopo))
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
    precision= 0.0;

    while (cnt < max_trials)   {
	cnt++;

	MPI_Barrier(comm);

	// Run one trial
	metric= do_one_trial(num_ops, msg_len, comm, ctopo);

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

    fprintf(stderr, "Usage: %s [-l len] [-a type] [-p precision]\n", pname);
    fprintf(stderr, "    -a type        Select type of allreduce algorithm. Default 0\n");
    fprintf(stderr, "                     0 is TREE_DEEP\n");
    fprintf(stderr, "                     1 is TREE_BINARY\n");
    fprintf(stderr, "                     2 is MPI library MPI_Allreduce()\n");
    fprintf(stderr, "    -l len         Size of allreduce operations in number of doubles. Default 1\n");
    fprintf(stderr, "    -p rpecision   Set precision (confidence interval). Default %.3f\n",
	DEFAULT_PRECISION);
    fprintf(stderr, "                   A precision of 0.0 disables sampling\n");

}  /* end of usage() */
