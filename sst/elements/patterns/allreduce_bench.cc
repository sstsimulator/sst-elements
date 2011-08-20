/*
** Copyright (c) 2011, IBM Corporation
** All rights reserved.
**
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
*/

#include <stdio.h>
#include <stdlib.h>	/* For strtol(), exit() */
#include <unistd.h>	/* For getopt() */
#include <mpi.h>
#include "collective_topology.cc"


/* Constants */
#define FALSE			(0)
#define TRUE			(1)
#define DEFAULT_MSG_LEN		(1)
#define DEFAULT_NUM_MSGS	(200)

#define MY_TAG_UP		(55)
#define MY_TAG_DOWN		(77)
#define MY_ALLREDUCE_TYPE	double
#define MY_ALLREDUCE_MPI_TYPE	MPI_DOUBLE


/* Local functions */
static double Test1(int num_msgs, int msg_len);
static double Test2(int num_msgs, Collective_topology *ctopo);
static void usage(char *pname);
MY_ALLREDUCE_TYPE my_allreduce(MY_ALLREDUCE_TYPE in, Collective_topology *ctopo);



int
main(int argc, char *argv[])
{

int ch, error;
int my_rank, num_ranks;
int msg_len;
int num_msgs;
int i;
double duration;
double total_time;
Collective_topology *ctopo;



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
    num_msgs= DEFAULT_NUM_MSGS;
    msg_len= DEFAULT_MSG_LEN;


    /* Check command line args */
    while ((ch= getopt(argc, argv, ":l:n:")) != EOF)   {
        switch (ch)   {
            case 'l':
		msg_len= strtol(optarg, (char **)NULL, 0);
		if (msg_len < 1)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "message length (-l) must be > 0!\n");
		    }
		    error= TRUE;
		}
		break;
            case 'n':
		num_msgs= strtol(optarg, (char **)NULL, 0);
		if (num_msgs < 1)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Number of messages (-n) must be > 0!\n");
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

    MPI_Barrier(MPI_COMM_WORLD);
    duration= Test1(num_msgs, msg_len);
    MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    if (my_rank == 0)   {
	printf("#  |||  Test 1: MPI_Allreduce() average time %12.9f\n",
	    (total_time / num_ranks) / num_msgs);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    duration= Test2(num_msgs, ctopo);
    MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    if (my_rank == 0)   {
	printf("#  |||  Test 2: my_allreduce() average time  %12.9f\n",
	    (total_time / num_ranks) / num_msgs);
    }

    MPI_Finalize();

    return 0;

}  /* end of main() */



static double
Test1(int num_msgs, int msg_len)
{

double t;
int i;
double sbuf[msg_len];
double rbuf[msg_len];


	/* Start the timer */
	t= MPI_Wtime();
	for (i= 0; i < num_msgs; i++)   {
	    MPI_Allreduce(sbuf, rbuf, msg_len, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	}

	return MPI_Wtime() - t;

}  /* end of Test1() */



static double
Test2(int num_msgs, Collective_topology *ctopo)
{

double t;
int i;
MY_ALLREDUCE_TYPE in;


	/* Start the timer */
	t= MPI_Wtime();
	for (i= 0; i < num_msgs; i++)   {
	    my_allreduce(in, ctopo);
	}

	return MPI_Wtime() - t;

}  /* end of Test2() */



MY_ALLREDUCE_TYPE
my_allreduce(MY_ALLREDUCE_TYPE in, Collective_topology *ctopo)
{

MY_ALLREDUCE_TYPE result;
MY_ALLREDUCE_TYPE tmp;
std::list<int>::iterator it;


    if (ctopo->is_leaf())   {
	// Send data to parent and wait for result
	MPI_Send(&in, 1, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_UP, MPI_COMM_WORLD);
	MPI_Recv(&result, 1, MY_ALLREDUCE_MPI_TYPE, ctopo->parent_rank(), MY_TAG_DOWN,
		MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else   {
	// Wait for the children
	result= in;
	for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
	    MPI_Recv(&tmp, 1, MY_ALLREDUCE_MPI_TYPE, *it, MY_TAG_UP, MPI_COMM_WORLD,
		    MPI_STATUS_IGNORE);
	    result= result + tmp;
	}

	if (!ctopo->is_root())   {
	    // Send it to the parent and wait for answer
	    MPI_Send(&result, 1, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_UP,
		    MPI_COMM_WORLD);
	    MPI_Recv(&result, 1, MY_ALLREDUCE_MPI_TYPE, ctopo->parent_rank(),
		    MY_TAG_DOWN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	// Send it back out
	for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
	    MPI_Send(&result, 1, MY_ALLREDUCE_MPI_TYPE, *it, MY_TAG_DOWN,
		    MPI_COMM_WORLD);
	}
    }

    return result;

}  /* end of my_allreduce() */



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-l len] [-n num]\n", pname);
    fprintf(stderr, "    -l len      Message length (number of doubles). Default %d\n", DEFAULT_MSG_LEN);
    fprintf(stderr, "    -n num      Number of messages per test. Default %d\n", DEFAULT_NUM_MSGS);

}  /* end of usage() */
