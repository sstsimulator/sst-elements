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

// FIXME: Don't like to include .cc files, but don't know how to fix the Makefile.am to avoid it
#include "collective_topology.cc"
#include "stats.cc"


/* Constants */
#define FALSE			(0)
#define TRUE			(1)
#define DEFAULT_NUM_OPS		(200)
#define DEFAULT_NUM_SETS	(9)

#define MY_TAG_UP		(55)
#define MY_TAG_DOWN		(77)



/* Local functions */
static double Test1(int num_ops, int msg_len, MPI_Comm comm);
static double Test2(int num_ops, Collective_topology *ctopo, int msg_len);
void my_allreduce(double *in, double *result, int msg_len, Collective_topology *ctopo);
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
int msg_len;
int set;
std::list <double>times;
int library;
tree_type_t tree;



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
    tree= TREE_DEEP;


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
	    case 't':
		if (strtol(optarg, (char **)NULL, 0) == 0)   {
		    tree= TREE_DEEP;
		} else if (strtol(optarg, (char **)NULL, 0) == 1)   {
		    tree= TREE_BINARY;
		} else   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Unknown tree type (-t) must be 0 or 1!\n");
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

    ctopo= new Collective_topology(my_rank, num_ranks, tree);

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
	duration= Test1(num_ops, msg_len, MPI_COMM_WORLD);
	MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	// Record the average time per allreduce
	times.push_back(total_time / num_ranks / num_ops);
    }
    if (my_rank == 0)   {
	printf("#  |||  %d operations per set. %d sets per node size. %d byte msg len\n",
		num_ops, num_sets, msg_len * (int)sizeof(double));
	if (tree == TREE_BINARY)   {
	    printf("#  |||  my_allreduce() uses binary tree\n");
	} else if (tree == TREE_DEEP)   {
	    printf("#  |||  my_allreduce() uses deep tree\n");
	} else   {
	    printf("#  |||  my_allreduce() uses unknown tree type\n");
	}
	printf("#  |||  Test 1: MPI_Allreduce() min, mean, median, max, sd\n");
	printf("#      ");
	print_stats(times);
    }



    // Test 2
    times.clear();
    for (set= 0; set < num_sets; set++)   {
	MPI_Barrier(MPI_COMM_WORLD);
	duration= Test2(num_ops, ctopo, msg_len);
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
	if (library)   {
	    printf("#  |||  Test 3: MPI_allreduce() nodes, min, mean, median, max, sd\n");
	} else   {
	    printf("#  |||  Test 3: my_allreduce() nodes, min, mean, median, max, sd\n");
	}
    }

    for (nnodes= 1; nnodes <= num_ranks; nnodes++)   {
	MPI_Group world_group, new_group;
	MPI_Comm new_comm;
	int ranges[][3]={{0, nnodes - 1, 1}};
	int group_size;

	MPI_Comm_group(MPI_COMM_WORLD, &world_group);
	MPI_Group_range_incl(world_group, 1, ranges, &new_group);
	MPI_Group_size(new_group, &group_size);
	MPI_Comm_create(MPI_COMM_WORLD, new_group, &new_comm);

	delete ctopo;
	ctopo= new Collective_topology(my_rank, nnodes, tree);
	times.clear();
	for (set= 0; set < num_sets; set++)   {
	    MPI_Barrier(MPI_COMM_WORLD);
	    if (my_rank < nnodes)   {
		if (library)   {
		    // Use the built-in MPI_Allreduce
		    duration= Test1(num_ops, msg_len, new_comm);
		} else   {
		    // Use my allreduce
		    duration= Test2(num_ops, ctopo, msg_len);
		}
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
Test1(int num_ops, int msg_len, MPI_Comm comm)
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
	memset(sbuf, 0, sizeof(double) * msg_len);
	memset(rbuf, 0, sizeof(double) * msg_len);
	
	/* Start the timer */
	t= MPI_Wtime();
	for (i= 0; i < num_ops; i++)   {
	    MPI_Allreduce(sbuf, rbuf, msg_len, MPI_DOUBLE, MPI_SUM, comm);
	}

	t2= MPI_Wtime() - t;
	free(sbuf);
	free(rbuf);
	return t2;

}  /* end of Test1() */



static double
Test2(int num_ops, Collective_topology *ctopo, int msg_len)
{

double t;
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
	memset(sbuf, 0, sizeof(double) * msg_len);
	memset(rbuf, 0, sizeof(double) * msg_len);

	/* Start the timer */
	t= MPI_Wtime();
	for (i= 0; i < num_ops; i++)   {
	    my_allreduce(sbuf, rbuf, msg_len, ctopo);
	}

	return MPI_Wtime() - t;

}  /* end of Test2() */



// Instead of using the MPI provided allreduce, we use this algorithm.
// It is the same one used in the communication pattern and allows for
// more accurate comparisons, since the number of messages as well as
// sources and destinations are the same.
void
my_allreduce(double *in, double *result, int msg_len, Collective_topology *ctopo)
{

double *tmp;
std::list<int>::iterator it;


    if (ctopo->is_leaf() && ctopo->num_nodes() > 1)   {
	// Send data to parent and wait for result
	MPI_Send(in, msg_len, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_UP, MPI_COMM_WORLD);
	MPI_Recv(result, msg_len, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_DOWN,
		MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else   {
	// Wait for the children
	memcpy(result, in, msg_len * sizeof(double));

	// We could do this outside of the loop, but that would probably not be fair
	// to MPI_Allreduce()
	tmp= (double *)malloc(msg_len * sizeof(double));
	if (tmp == NULL)   {
	    fprintf(stderr, "Out of memory!\n");
	    exit(-1);
	}

	for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
	    MPI_Recv(tmp, msg_len, MPI_DOUBLE, *it, MY_TAG_UP, MPI_COMM_WORLD,
		    MPI_STATUS_IGNORE);
	    for (int i= 0; i < msg_len; i++)   {
		result[i]= result[i] + tmp[i];
	    }
	}

	if (!ctopo->is_root())   {
	    // Send it to the parent and wait for answer
	    MPI_Send(result, msg_len, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_UP,
		    MPI_COMM_WORLD);
	    MPI_Recv(result, msg_len, MPI_DOUBLE, ctopo->parent_rank(),
		    MY_TAG_DOWN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	// Send it back out
	for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
	    MPI_Send(result, msg_len, MPI_DOUBLE, *it, MY_TAG_DOWN,
		    MPI_COMM_WORLD);
	}

	free(tmp);
    }

}  /* end of my_allreduce() */



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-b] [-l len] [-n ops] [-s sets] [-t type]\n", pname);
    fprintf(stderr, "    -b          Use the MPI library MPI_Allreduce\n");
    fprintf(stderr, "    -l len      Size of allreduce operations in number of doubles. Default 1\n");
    fprintf(stderr, "    -n ops      Number of allreduce operations per test. Default %d\n",
	DEFAULT_NUM_OPS);
    fprintf(stderr, "    -s sets     Number of sets; i.e. number of test repeats. Default %d\n",
	DEFAULT_NUM_SETS);
    fprintf(stderr, "    -t type     Select type of tree of allreduce algorithm. Default 0\n");
    fprintf(stderr, "                0 is TREE_DEEP\n");
    fprintf(stderr, "                1 is TREE_BINARY\n");

}  /* end of usage() */
