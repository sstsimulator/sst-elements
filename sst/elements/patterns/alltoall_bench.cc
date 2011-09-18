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
#include <unistd.h>	/* For getopt() */
#include <math.h>
#include <mpi.h>

// FIXME: Don't like to include .cc files, but don't know how to fix the Makefile.am to avoid it
#include "stats.cc"


/* Constants */
#define FALSE			(0)
#define TRUE			(1)
#define DEFAULT_NUM_OPS		(200)
#define DEFAULT_NUM_SETS	(9)




/* Local functions */
static double Test1(int num_ops, int msg_len, int nranks, MPI_Comm comm);
static double Test2(int num_ops, int nranks, int msg_len);
void my_alltoall(double *in, double *result, int msg_len, int nranks);
void print_stats(std::list<double> t);
static void usage(char *pname);


/* Make these two global */
static int my_rank, num_ranks;



int
main(int argc, char *argv[])
{

int ch, error;
int num_ops;
int i;
double duration;
double total_time;
int nnodes;
int num_sets;
int msg_len;
int set;
std::list <double>times;
int library;



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
    error= FALSE;
    num_ops= DEFAULT_NUM_OPS;
    num_sets= DEFAULT_NUM_SETS;
    msg_len= 1;
    library= 0;


    /* Check command line args */
    while ((ch= getopt(argc, argv, "bl:n:s:t:")) != EOF)   {
        switch (ch)   {
            case 'b':
		library= 1;
		break;
            case 'l':
		msg_len= strtol(optarg, (char **)NULL, 0);
		if (msg_len < 1)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Message length in doubles (-l) must be > 0!\n");
		    }
		    error= TRUE;
		}
		break;
            case 'n':
		num_ops= strtol(optarg, (char **)NULL, 0);
		if (num_ops < 1)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Number of alltoall operations (-n) must be > 0!\n");
		    }
		    error= TRUE;
		}
		break;
            case 's':
		num_sets= strtol(optarg, (char **)NULL, 0);
		if (num_sets < 1)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Number of sets (-s) must be > 0!\n");
		    }
		    error= TRUE;
		}
		break;

	    /* Command line error checking */
            case '?':
		if (my_rank == 0)   {
		    fprintf(stderr, "Unknown option \"%s\"\n", argv[optind - 1]);
		}
		error= TRUE;
		break;
            case ':':
		if (my_rank == 0)   {
		    fprintf(stderr, "Missing option argument to \"%s\"\n", argv[optind - 1]);
		}
		error= TRUE;
		break;
            default:
		error= TRUE;
		break;
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
	printf("# -------------------\n");
	printf("# Command line \"");
	for (i= 0; i < argc; i++)   {
	    printf("%s", argv[i]);
	    if (i < (argc - 1))   {
		printf(" ");
	    }
	}
	printf("\"\n");
    }



    // Test 1
    times.clear();
    for (set= 0; set < num_sets; set++)   {
	MPI_Barrier(MPI_COMM_WORLD);
	duration= Test1(num_ops, msg_len, num_ranks, MPI_COMM_WORLD);
	MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	// Record the average time per alltoall
	times.push_back(total_time / num_ranks / num_ops);
    }
    if (my_rank == 0)   {
	printf("#  |||  %d operations per set. %d sets per node size. %d byte msg len\n",
		num_ops, num_sets, msg_len * (int)sizeof(double));
	printf("#  |||  Test 1: MPI_Alltoall() min, mean, median, max, sd\n");
	printf("#      ");
	print_stats(times);
    }



    // Test 2
    times.clear();
    for (set= 0; set < num_sets; set++)   {
	MPI_Barrier(MPI_COMM_WORLD);
	duration= Test2(num_ops, num_ranks, msg_len);
	MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	// Record the average time per alltoall
	times.push_back(total_time / num_ranks / num_ops);
    }
    if (my_rank == 0)   {
	printf("#  |||  Test 2: my_alltoall() min, mean, median, max, sd\n");
	printf("#      ");
	print_stats(times);
    }


    // Test 3
    // Now we do this with increasing number of ranks, instead of all of them
    if (my_rank == 0)   {
	if (library)   {
	    printf("#  |||  Test 3: MPI_Alltoall() nodes, min, mean, median, max, sd\n");
	} else   {
	    printf("#  |||  Test 3: my_alltoall() nodes, min, mean, median, max, sd\n");
	}
    }

    for (nnodes= 1; nnodes <= num_ranks; nnodes= nnodes << 1)   {
	MPI_Group world_group, new_group;
	MPI_Comm new_comm;
	int ranges[][3]={{0, nnodes - 1, 1}};
	int group_size;

	MPI_Comm_group(MPI_COMM_WORLD, &world_group);
	MPI_Group_range_incl(world_group, 1, ranges, &new_group);
	MPI_Group_size(new_group, &group_size);
	MPI_Comm_create(MPI_COMM_WORLD, new_group, &new_comm);

	times.clear();
	for (set= 0; set < num_sets; set++)   {
	    MPI_Barrier(MPI_COMM_WORLD);
	    if (my_rank < nnodes)   {
		if (library)   {
		    // Use the built-in MPI_Alltoall
		    duration= Test1(num_ops, msg_len, nnodes, new_comm);
		} else   {
		    // Use my alltoall
		    duration= Test2(num_ops, nnodes, msg_len);
		}
	    } else   {
		duration= 0.0;
	    }
	    MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	    // Record the average time per alltoall
	    times.push_back(total_time / nnodes / num_ops);
	}
	if (my_rank == 0)   {
	    printf("%6d ", nnodes);
	    print_stats(times);
	}
    }

    MPI_Finalize();

    return 0;

}  /* end of main() */



static double
Test1(int num_ops, int msg_len, int nranks, MPI_Comm comm)
{

double t, t2;
int i, j;
double *sbuf;
double *rbuf;


	// Allocate memory
	sbuf= (double *)malloc(sizeof(double) * msg_len * nranks);
	rbuf= (double *)malloc(sizeof(double) * msg_len * nranks);
	if ((sbuf == NULL) || (rbuf == NULL))   {
	    fprintf(stderr, "Out of memory!\n");
	    exit(-1);
	}
	for (j= 0; j < nranks; j++)   {
	    for (i= 0; i < msg_len; i++)   {
		sbuf[j * msg_len + i]= i + my_rank;
	    }
	}

	memset(rbuf, 0, sizeof(double) * msg_len * nranks);
	
	/* Start the timer */
	t= MPI_Wtime();
	for (i= 0; i < num_ops; i++)   {
	    MPI_Alltoall(sbuf, msg_len, MPI_DOUBLE, rbuf, msg_len, MPI_DOUBLE, comm);
	}

	t2= MPI_Wtime() - t;

	/* Check */
	for (j= 0; j < nranks; j++)   {
	    for (i= 0; i < msg_len; i++)   {
		if (rbuf[j * msg_len + i] != i + j)   {
		    fprintf(stderr, "[%3d] MPI_Alltoall() failed at rbuf[j %d, i %d]\n", my_rank, j, i);
		}
	    }
	}

	free(sbuf);
	free(rbuf);
	return t2;

}  /* end of Test1() */



static double
Test2(int num_ops, int nranks, int msg_len)
{

double t;
int i, j;
double *sbuf;
double *rbuf;


	// Allocate memory
	sbuf= (double *)malloc(sizeof(double) * msg_len * nranks);
	rbuf= (double *)malloc(sizeof(double) * msg_len * nranks);
	if ((sbuf == NULL) || (rbuf == NULL))   {
	    fprintf(stderr, "Out of memory!\n");
	    exit(-1);
	}
	for (j= 0; j < nranks; j++)   {
	    for (i= 0; i < msg_len; i++)   {
		sbuf[j * msg_len + i]= i + my_rank;
	    }
	}

	memset(rbuf, 0, sizeof(double) * msg_len * nranks);
	
	/* Start the timer */
	t= MPI_Wtime();
	for (i= 0; i < num_ops; i++)   {
	    my_alltoall(sbuf, rbuf, msg_len, nranks);
	}

	/* Check */
	for (j= 0; j < nranks; j++)   {
	    for (i= 0; i < msg_len; i++)   {
		if (rbuf[j * msg_len + i] != i + j)   {
		    fprintf(stderr, "[%3d] my_alltoall() failed at rbuf[j %d, i %d]\n", my_rank, j, i);
		}
	    }
	}

	free(sbuf);
	free(rbuf);
	return MPI_Wtime() - t;

}  /* end of Test2() */



// Instead of using the MPI provided alltoall, we use this algorithm.
// It is the same one used in the communication pattern and allows for
// more accurate comparisons, since the number of messages as well as
// sources and destinations are the same.
//
// NOTE! This only works for powers of 2!!!
void
my_alltoall(double *in, double *result, int msg_len, int nranks)
{

unsigned int i;
int shift, dest;
int src;
int offset;
int len1, len2;
MPI_Request sreq1, sreq2;
MPI_Request rreq1, rreq2;


    sreq1= MPI_REQUEST_NULL;
    sreq2= MPI_REQUEST_NULL;
    rreq1= MPI_REQUEST_NULL;
    rreq2= MPI_REQUEST_NULL;

    /* My own contribution */
    for (i= 0; i < msg_len; i++)   {
	result[my_rank * msg_len + i]= in[my_rank * msg_len + i];
    }

    i= nranks >> 1;
    shift= 1;
    while (i > 0)   {
	dest= (my_rank + shift) % nranks;
	src= ((my_rank - shift) + nranks) % nranks;

	offset= (my_rank * msg_len) - ((shift - 1) * msg_len);
	if (offset < 0)   {
	    /* Need to break it up into two pieces */
	    offset= offset + (nranks * msg_len);
	    len1= (nranks * msg_len) - offset;
	    MPI_Isend(result + offset, len1, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD, &sreq1);
	    len2= shift * msg_len - (nranks * msg_len - offset);
	    MPI_Isend(result, len2, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD, &sreq2);
	} else   {
	    /* I can send it in one piece */
	    MPI_Isend(result + offset, shift * msg_len, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD, &sreq1);
	}

	offset= (src * msg_len) - ((shift - 1) * msg_len);
	if (offset < 0)   {
	    /* Need to break it up into two pieces */
	    offset= offset + (nranks * msg_len);
	    len1= (nranks * msg_len) - offset;
	    MPI_Irecv(result + offset, len1, MPI_DOUBLE, src, 0, MPI_COMM_WORLD, &rreq1);
	    len2= shift * msg_len - (nranks * msg_len - offset);
	    MPI_Irecv(result, len2, MPI_DOUBLE, src, 0, MPI_COMM_WORLD, &rreq2);
	} else   {
	    /* I can receive it in one piece */
	    MPI_Irecv(result + offset, shift * msg_len, MPI_DOUBLE, src, 0, MPI_COMM_WORLD, &rreq1);
	}

	/* End of loop housekeeping */
	shift= shift << 1;
	i= i >> 1;
	MPI_Wait(&rreq1, MPI_STATUS_IGNORE);
	MPI_Wait(&sreq1, MPI_STATUS_IGNORE);
	if (rreq2 != MPI_REQUEST_NULL)   {
	    MPI_Wait(&rreq2, MPI_STATUS_IGNORE);
	}
	if (sreq2 != MPI_REQUEST_NULL)   {
	    MPI_Wait(&sreq2, MPI_STATUS_IGNORE);
	}
    }

}  /* end of my_alltoall() */



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-b] [-l len] [-n ops] [-s sets]\n", pname);
    fprintf(stderr, "    -b          Use the MPI library MPI_Alltoall\n");
    fprintf(stderr, "    -l len      Size of alltoall operations in number of doubles. Default 1\n");
    fprintf(stderr, "    -n ops      Number of alltoall operations per test. Default %d\n",
	DEFAULT_NUM_OPS);
    fprintf(stderr, "    -s sets     Number of sets; i.e. number of test repeats. Default %d\n",
	DEFAULT_NUM_SETS);

}  /* end of usage() */
