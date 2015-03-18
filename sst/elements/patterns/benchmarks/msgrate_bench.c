/*
** $Id:$
**
** Rolf Riesen, Sandia National Laboratories, February 2011
**
** A communication benchmark that mimics the behavior of the
** msgrate_pattern module for SST using MPI.
**
**
** Copyright 2009-2015 Sandia Corporation. Under the terms
** of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
** Government retains certain rights in this software.
**
** Copyright (c) 2009-2015, Sandia Corporation
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
#include "stat_p.h"
#include "util.h"


/* Constants */
#define FALSE			(0)
#define TRUE			(1)
#define DEFAULT_MSG_LEN		(0)
#define DEFAULT_NUM_MSGS	(200)
#define MAX_MSG_LEN		(10 * 1024 * 1024)
#define DEFAULT_STRIDE		(1)


/* Local functions */
static void usage(char *pname, int start_rank_default);
static double Test1(int my_rank, int num_ranks, int num_msgs, int msg_len);
static double Test2(int my_rank, int num_ranks, int num_msgs, int msg_len, int start_rank, int stride);
static double Test3(int my_rank, int num_ranks, int num_msgs, int msg_len, int start_rank, int stride);


/* Storage */
char buf[MAX_MSG_LEN];



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
int start_rank;
int stride;
int num_sender;
int stat_mode;
double req_precision;
int ii;
double precision;
int trials;
double tot;
double tot_squared;
double avg_bisection_rate;
double metric;



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

    if (num_ranks % 2 != 0)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Need to run on an even number of ranks\n");
	}
	MPI_Finalize();
	exit(-1);
    }

    /* Set the defaults */
    opterr= 0;		/* Disable getopt error printing */
    error= FALSE;
    num_msgs= DEFAULT_NUM_MSGS;
    msg_len= DEFAULT_MSG_LEN;
    start_rank= num_ranks / 2;
    stride= DEFAULT_STRIDE;
    stat_mode= FALSE;
    req_precision= 0.01;



    /* Check command line args */
    while ((ch= getopt(argc, argv, ":l:n:s:i:p:t")) != EOF)   {
        switch (ch)   {
            case 'l':
		msg_len= strtol(optarg, (char **)NULL, 0);
		if (msg_len < 0)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "message length (-l) must be >= 0!\n");
		    }
		    error= TRUE;
		}
		if (msg_len > MAX_MSG_LEN)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "message length (-l) must be <= %d!\n", MAX_MSG_LEN);
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

            case 's':
		start_rank= strtol(optarg, (char **)NULL, 0);
		if ((start_rank < 1) || (start_rank > num_ranks))   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Start node (-s) must be > 0 and < n - 1!\n");
		    }
		    error= TRUE;
		}
		break;

            case 'i':
		stride= strtol(optarg, (char **)NULL, 0);
		if ((stride < 1) || (stride > num_ranks))   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Node stride (-i) must be > 0 and < n - 1!\n");
		    }
		    error= TRUE;
		}
		break;

            case 'p':
		req_precision= strtod(optarg, (char **)NULL);
		if (my_rank == 0)   {
		    printf("# req_precision is %f\n", req_precision);
		}
		break;

	    case 't':
		stat_mode= TRUE;
		break;

	    /* Command line error checking */
	    DEFAULT_CMD_LINE_ERR_CHECK
        }
    }
 
    if (num_msgs < 2)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Number of messages must be > 1; e.g., %d\n", DEFAULT_NUM_MSGS);
	}
	error= TRUE;
    }

    if (error)   {
        if (my_rank == 0)   {
            usage(argv[0], num_ranks / 2);
        }
	MPI_Finalize();
        exit (-1);
    }

    if (my_rank == 0)   {
	printf("Message Rate Pattern\n");
	printf("--------------------\n");
	printf("Command line \"");
	for (i= 0; i < argc; i++)   {
	    printf("%s", argv[i]);
	    if (i < (argc - 1))   {
		printf(" ");
	    }
	}
	printf("\"\n");
    }

    num_sender= num_ranks / 2;

    ii= 0;
    if (stat_mode)   {
	trials= 10000;
    } else   {
	trials= 20;
    }

    tot= 0.0;
    tot_squared= 0.0;

    while (ii < trials)   {
	duration = Test1(my_rank, num_ranks, num_msgs, msg_len);
	MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	avg_bisection_rate= 1.0 / ((total_time / (num_ranks / 2)) / num_msgs);
	tot= tot + avg_bisection_rate;
	tot_squared= tot_squared + avg_bisection_rate * avg_bisection_rate;
	precision= stat_p(ii + 1, tot, tot_squared);
	if (stat_mode)   {
	    /* check for precision if at least 3 trials have taken place. ii > 1 => N > 2. */
	    if (my_rank == 0 && ii > 1 && precision <= req_precision)   {
		trials= ii;
	    }
	    MPI_Bcast(&trials, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}
	ii++;
    }

    if (stat_mode) {
	/* if i = 10, actual trials done is 11 */
	trials++; /* this is the total number of trials that took place */
    }


    if (my_rank == 0)   {
	printf("#  |||  Test 1: Ranks 0...%d will send %d messages of length %d to "
	    "ranks %d...%d along %d paths\n", num_ranks / 2 - 1, num_msgs,
	    msg_len, num_ranks / 2, num_ranks - 1, num_ranks / 2);
	printf("#  |||  Test 1: %d senders, %d receivers\n", num_sender, num_sender);
	printf("#  |||  Test 1: Average bi-section rate: %8.0f msgs/s\n",
	    tot/trials );
	printf("#  |||  Test 1: Precision of %f, averaged over %d trials\n",
	    precision, trials );
    }


    num_sender= ((num_ranks - start_rank) / stride);
    if ((num_ranks - start_rank) % stride != 0)   {
	num_sender++;
    }

    ii= 0;
    if (stat_mode)   {
	trials= 10000;
    } else   {
        trials= 20;
    }

    tot= 0.0;
    tot_squared= 0.0;
    while  (ii < trials)    {
	duration= Test2(my_rank, num_ranks, num_msgs, msg_len, start_rank, stride);
	MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	metric= 1.0 / ((total_time / num_sender) / (num_msgs - 1));
	tot= tot + metric;
	tot_squared= tot_squared + metric*metric;
	precision= stat_p(ii + 1, tot, tot_squared);

	if (stat_mode) {
	    /* check for precision if at least 3 trials have taken place. ii > 1 => N > 2.  */
	    if (my_rank == 0 && ii > 1 && precision <= req_precision)   {
		trials= ii;
	    }
	    MPI_Bcast(&trials, 1, MPI_INT, 0, MPI_COMM_WORLD); 
	}
	ii++;
    }

    if (stat_mode)   {
        /* if i = 10, actual trials done is 11 */
        trials++; /* this is the total number of trials that took place */
    }


    if (my_rank == 0)   {
	printf("#  |||  Test 2: Rank 0 will send %d messages of length %d to ranks %d...%d, stride %d\n",
	    num_msgs, msg_len, start_rank, num_ranks - 1, stride);
	printf("#  |||  Test 2: %d receivers\n", num_sender);
	printf("#  |||  Test 2: Average send rate:       %8.0f msgs/s\n",
        tot/trials );
	printf("#  |||  Test 2: Precision of %f, averaged over %d trials\n",
	    precision, trials );
    }

    ii= 0;
    if (stat_mode)   {
        trials= 10000;
    } else   {
        trials= 20;
    }

    tot= 0.0; 
    tot_squared= 0.0; 
    while  (ii < trials)   {
	duration= Test3(my_rank, num_ranks, num_msgs, msg_len, start_rank, stride);
	MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	metric= 1.0 / (total_time / (num_msgs * num_sender - 1));
	tot= tot + metric;
	tot_squared= tot_squared + metric * metric;
	precision= stat_p(ii + 1, tot, tot_squared);

	if (stat_mode) {
	    /* check for precision if at least 3 trials have taken place. ii > 1 => N > 2.  */
	    if (my_rank == 0 && ii > 1 && precision <= req_precision)   {
		trials = ii;
	    }
	    MPI_Bcast(&trials, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}
	ii++;
    }

    if (stat_mode) {
        /* if i = 10, actual trials done is 11 */
        trials++; /* this is the total number of trials that took place */
    }

    if (my_rank == 0)   {
	printf("#  |||  Test 3: Ranks %d...%d, stride %d, will send %d messages of length %d to rank 0\n",
	    start_rank, num_ranks - 1, stride, num_msgs, msg_len);
	printf("#  |||  Test 3: %d senders\n", num_sender);
	printf("#  |||  Test 3: Average receive rate:    %8.0f msgs/s\n",
	    tot/trials);
	printf("#  |||  Test 3: Precision of %f, averaged over %d trials\n",
	    precision, trials );
    }

    MPI_Finalize();

    return 0;

}  /* end of main() */



static double
Test1(int my_rank, int num_ranks, int num_msgs, int msg_len)
{

double t;
int i;
MPI_Request req;


    if (my_rank < num_ranks / 2)   {
	/* I'm a sender */
	MPI_Barrier(MPI_COMM_WORLD);
	for (i= 0; i < num_msgs; i++)   {
	    MPI_Send(buf, msg_len, MPI_CHAR, my_rank + num_ranks / 2, 12, MPI_COMM_WORLD);
	}
	return 0.0;

    } else   {

	/* I'm a receiver, wait for the first message */
	MPI_Irecv(buf, msg_len, MPI_CHAR, my_rank - num_ranks / 2, 12, MPI_COMM_WORLD, &req);
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Wait(&req, MPI_STATUS_IGNORE);

	/* Now start the timer */
	t= MPI_Wtime();
	for (i= 1; i < num_msgs; i++)   {
	    MPI_Recv(buf, msg_len, MPI_CHAR, my_rank - num_ranks / 2, 12, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	return MPI_Wtime() - t;
    }

}  /* end of Test1() */



static double
Test2(int my_rank, int num_ranks, int num_msgs, int msg_len, int start_rank, int stride)
{

double t;
int i;
int dest;
MPI_Request req;
int num_receivers;


    num_receivers= ((num_ranks - start_rank) / stride);
    if ((num_ranks - start_rank) % stride != 0)   {
	num_receivers++;
    }

    if (my_rank == 0)   {
	/* I'm the sender */
	dest= start_rank;
	MPI_Barrier(MPI_COMM_WORLD);

	/* Send num_msgs round robin to each of the receivers */
	for (i= 0; i < num_msgs * num_receivers; i++)   {
	    MPI_Send(buf, msg_len, MPI_CHAR, dest, 13, MPI_COMM_WORLD);
	    dest= dest + stride;
	    if (dest >= num_ranks)   {
		dest= start_rank;
	    }
	}
	return 0.0;

    } else if ((my_rank >= start_rank) && ((my_rank - start_rank) % stride) == 0)   {
	/* I am a receiver */
	MPI_Irecv(buf, msg_len, MPI_CHAR, 0, 13, MPI_COMM_WORLD, &req);
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Wait(&req, MPI_STATUS_IGNORE);
	t= MPI_Wtime();
	for (i= 1; i < num_msgs; i++)   {
	    MPI_Recv(buf, msg_len, MPI_CHAR, 0, 13, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	return MPI_Wtime() - t;
    } else   {
	MPI_Barrier(MPI_COMM_WORLD);
	return 0.0;
    }

}  /* end of Test2() */



static double
Test3(int my_rank, int num_ranks, int num_msgs, int msg_len, int start_rank, int stride)
{

double t;
int i;
int dest;
int num_sender;
MPI_Request req;


    if (my_rank == 0)   {
	/* I'm the receiver */
	num_sender= ((num_ranks - start_rank) / stride);
	if ((num_ranks - start_rank) % stride != 0)   {
	    num_sender++;
	}

	MPI_Irecv(buf, msg_len, MPI_CHAR, MPI_ANY_SOURCE, 14, MPI_COMM_WORLD, &req);
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Wait(&req, MPI_STATUS_IGNORE);
	t= MPI_Wtime();
	for (i= 1; i < num_msgs * num_sender; i++)   {
	    MPI_Recv(buf, msg_len, MPI_CHAR, MPI_ANY_SOURCE, 14, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	return MPI_Wtime() - t;
    } else if ((my_rank >= start_rank) && ((my_rank - start_rank) % stride) == 0)   {
	/* I'm a sender */
	dest= 0;
	MPI_Barrier(MPI_COMM_WORLD);
	for (i= 0; i < num_msgs; i++)   {
	    MPI_Send(buf, msg_len, MPI_CHAR, dest, 14, MPI_COMM_WORLD);
	}
    } else   {
	MPI_Barrier(MPI_COMM_WORLD);
    }

    return 0.0;

}  /* end of Test3() */



static void
usage(char *pname, int start_rank_default)
{

    fprintf(stderr, "Usage: %s [-l len] [-n num] [-s start] [-i stride]\n", pname);
    fprintf(stderr, "    -l len      Message length. Default %d\n", DEFAULT_MSG_LEN);
    fprintf(stderr, "    -n num      Number of messages per test. Default %d\n", DEFAULT_NUM_MSGS);
    fprintf(stderr, "    -s start    Rank 0 sends/recvs from \"start\" to n. Default %d\n",
	    start_rank_default);
    fprintf(stderr, "    -i stride   Node stride. Default %d\n", DEFAULT_STRIDE);

}  /* end of usage() */
