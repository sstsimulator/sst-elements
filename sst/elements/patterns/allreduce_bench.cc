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
** Test 2 measures my_allreduce to compare againts test 1
*/

#include <stdio.h>
#include <stdlib.h>	/* For strtol(), exit() */
#include <unistd.h>	/* For getopt() */
#include <math.h>
#include <mpi.h>
#include "collective_topology.cc"


/* Constants */
#define FALSE			(0)
#define TRUE			(1)
#define DEFAULT_NUM_OPS		(200)
#define DEFAULT_NUM_SETS	(9)

#define MY_TAG_UP		(55)
#define MY_TAG_DOWN		(77)



/* Local functions */
static double Test1(int num_ops);
static double Test2(int num_ops, Collective_topology *ctopo);
double my_allreduce(double in, Collective_topology *ctopo);
void print_stats(std::list<double> t);
static void usage(char *pname);



int
main(int argc, char *argv[])
{

int ch, error;
int my_rank, num_ranks;
int num_ops;
int i;
double duration;
double total_time;
Collective_topology *ctopo;
int nnodes;
int num_sets;
int set;
std::list <double>times;



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


    /* Check command line args */
    while ((ch= getopt(argc, argv, "n:s:")) != EOF)   {
        switch (ch)   {
            case 'n':
		num_ops= strtol(optarg, (char **)NULL, 0);
		if (num_ops < 1)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Number of allreduce operations (-n) must be > 0!\n");
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

    ctopo= new Collective_topology(my_rank, num_ranks);

    if (my_rank == 0)   {
	printf("# Allreduce benchmark\n");
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
	duration= Test1(num_ops);
	MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	// Record the average time per allreduce
	times.push_back(total_time / num_ranks / num_ops);
    }
    if (my_rank == 0)   {
	printf("#  |||  Test 1: MPI_Allreduce() min, mean, median, max, sd\n");
	printf("#      ");
	print_stats(times);
    }



    // Test 2
    times.clear();
    for (set= 0; set < num_sets; set++)   {
	MPI_Barrier(MPI_COMM_WORLD);
	duration= Test2(num_ops, ctopo);
	MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	// Record the average time per allreduce
	times.push_back(total_time / num_ranks / num_ops);
    }
    if (my_rank == 0)   {
	printf("#  |||  Test 2: my_allreduce() min, mean, median, max, sd\n");
	printf("#      ");
	print_stats(times);
    }



    // Test 3
    // Now we do this with increasing number of ranks, instead of all of them
    if (my_rank == 0)   {
	printf("#  |||  Test 3: my_allreduce() nodes, min, mean, median, max, sd\n");
    }
    for (nnodes= 1; nnodes <= num_ranks; nnodes++)   {
	delete ctopo;
	ctopo= new Collective_topology(my_rank, nnodes);
	times.clear();
	for (set= 0; set < num_sets; set++)   {
	    MPI_Barrier(MPI_COMM_WORLD);
	    if (my_rank < nnodes)   {
		duration= Test2(num_ops, ctopo);
	    } else   {
		duration= 0.0;
	    }
	    MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	    // Record the average time per allreduce
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
Test1(int num_ops)
{

double t;
int i;
double sbuf;
double rbuf;


	/* Start the timer */
	t= MPI_Wtime();
	sbuf= 0.0;
	for (i= 0; i < num_ops; i++)   {
	    MPI_Allreduce(&sbuf, &rbuf, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	}

	return MPI_Wtime() - t;

}  /* end of Test1() */



static double
Test2(int num_ops, Collective_topology *ctopo)
{

double t;
int i;
double sbuf;


	/* Start the timer */
	sbuf= 0.0;
	t= MPI_Wtime();
	for (i= 0; i < num_ops; i++)   {
	    my_allreduce(sbuf, ctopo);
	}

	return MPI_Wtime() - t;

}  /* end of Test2() */



double
my_allreduce(double in, Collective_topology *ctopo)
{

double result;
double tmp;
std::list<int>::iterator it;


    if (ctopo->is_leaf() && ctopo->num_nodes() > 1)   {
	// Send data to parent and wait for result
	MPI_Send(&in, 1, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_UP, MPI_COMM_WORLD);
	MPI_Recv(&result, 1, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_DOWN,
		MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else   {
	// Wait for the children
	result= in;
	for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
	    MPI_Recv(&tmp, 1, MPI_DOUBLE, *it, MY_TAG_UP, MPI_COMM_WORLD,
		    MPI_STATUS_IGNORE);
	    result= result + tmp;
	}

	if (!ctopo->is_root())   {
	    // Send it to the parent and wait for answer
	    MPI_Send(&result, 1, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_UP,
		    MPI_COMM_WORLD);
	    MPI_Recv(&result, 1, MPI_DOUBLE, ctopo->parent_rank(),
		    MY_TAG_DOWN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	// Send it back out
	for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
	    MPI_Send(&result, 1, MPI_DOUBLE, *it, MY_TAG_DOWN,
		    MPI_COMM_WORLD);
	}
    }

    return result;

}  /* end of my_allreduce() */



void
print_stats(std::list<double> t)
{

int cnt;
std::list<double>::iterator it;
int num_sets;
double min, avg, med, max, sd;


	t.sort();
	cnt= 0;
	num_sets= t.size();
	med= 0.0;
	avg= 0.0;
	for (it= t.begin(); it != t.end(); it++)   {
	    if (cnt == (num_sets / 2))   {
		med= *it;
	    }
	    avg= avg + *it;
	    cnt++;
	}
	avg= avg / num_sets;

	// Calulate the standard deviation
	sd= 0.0;
	for (it= t.begin(); it != t.end(); it++)   {
	    sd= sd + ((*it - avg) * (*it - avg));
	}
	sd= sqrt(sd / num_sets);
	min= t.front();
	max= t.back();

	printf("%12.9f %12.9f %12.9f %12.9f %12.9f\n", min, avg, med, max, sd);

}  /* end of stats() */



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-l len] [-n num]\n", pname);
    fprintf(stderr, "    -n num      Number of allreduce operations per test. Default %d\n",
	DEFAULT_NUM_OPS);
    fprintf(stderr, "    -s num      Number of sets; i.e. number of test repeats. Default %d\n",
	DEFAULT_NUM_SETS);

}  /* end of usage() */
