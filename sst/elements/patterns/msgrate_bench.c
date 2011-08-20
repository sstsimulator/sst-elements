/*
** $Id:$
**
** Rolf Riesen, Sandia National Laboratories, February 2011
**
** A communication benchmark that mimics the behavior of the
** msgrate_pattern module for SST using MPI.
**
**
** Copyright 2009-2011 Sandia Corporation. Under the terms
** of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
** Government retains certain rights in this software.
**
** Copyright (c) 2009-2011, Sandia Corporation
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


/* Constants */
#define FALSE			(0)
#define TRUE			(1)
#define DEFAULT_MSG_LEN		(0)
#define DEFAULT_NUM_MSGS	(200)
#define MAX_MSG_LEN		(10 * 1024 * 1024)


/* Local functions */
static void usage(char *pname);
static double Test1(int my_rank, int num_ranks, int num_msgs, int msg_len);
static double Test2(int my_rank, int num_ranks, int num_msgs, int msg_len);
static double Test3(int my_rank, int num_ranks, int num_msgs, int msg_len);
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


    /* Check command line args */
    while ((ch= getopt(argc, argv, ":l:n:")) != EOF)   {
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
 
    if (num_msgs < 2)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Number of messages must be > 1; e.g., %d\n", DEFAULT_NUM_MSGS);
	}
	error= TRUE;
    }

    if (error)   {
        if (my_rank == 0)   {
            usage(argv[0]);
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

    duration= Test1(my_rank, num_ranks, num_msgs, msg_len);
    MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    if (my_rank == 0)   {
	printf("#  |||  Test 1: Ranks 0...%d will send %d messages of length %d to "
	    "ranks %d...%d along %d paths\n", num_ranks / 2 - 1, num_msgs,
	    msg_len, num_ranks / 2, num_ranks - 1, num_ranks / 2);
	printf("#  |||  Test 1: Average bi-section rate: %8.0f msgs/s\n",
	    1.0 / ((total_time / (num_ranks / 2)) / num_msgs));
    }


    duration= Test2(my_rank, num_ranks, num_msgs, msg_len);
    MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    if (my_rank == 0)   {
	printf("#  |||  Test 2: Rank 0 will send %d messages of length %d to ranks %d...%d\n",
	    num_msgs, msg_len, my_rank + 1, num_ranks - 1);
	printf("#  |||  Test 2: Average send rate:       %8.0f msgs/s\n",
	    1.0 / ((total_time / (num_ranks - 1)) / num_msgs));
    }


    duration= Test3(my_rank, num_ranks, num_msgs, msg_len);
    MPI_Allreduce(&duration, &total_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    if (my_rank == 0)   {
	printf("#  |||  Test 3: Ranks 1...%d will send %d messages of length %d to rank 0\n",
	    num_ranks - 1, num_msgs, msg_len);
	printf("#  |||  Test 3: Average receive rate:    %8.0f msgs/s\n",
	    1.0 / (duration / (num_msgs * (num_ranks - 1))));
    }

    MPI_Finalize();

    return 0;

}  /* end of main() */



static double
Test1(int my_rank, int num_ranks, int num_msgs, int msg_len)
{

double t;
int i;


    if (my_rank < num_ranks / 2)   {
	/* I'm a sender */
	for (i= 0; i < num_msgs; i++)   {
	    MPI_Send(buf, msg_len, MPI_CHAR, my_rank + num_ranks / 2, 12, MPI_COMM_WORLD);
	}
	return 0.0;

    } else   {

	/* I'm a receiver, wait for the first message */
	MPI_Recv(buf, msg_len, MPI_CHAR, my_rank - num_ranks / 2, 12, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	/* Now start the timer */
	t= MPI_Wtime();
	for (i= 1; i < num_msgs; i++)   {
	    MPI_Recv(buf, msg_len, MPI_CHAR, my_rank - num_ranks / 2, 12, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	return MPI_Wtime() - t;
    }

}  /* end of Test1() */



static double
Test2(int my_rank, int num_ranks, int num_msgs, int msg_len)
{

double t;
int i;
int dest;


    if (my_rank == 0)   {
	/* I'm the sender */
	dest= 1;
	for (i= 0; i < num_msgs * (num_ranks - 1); i++)   {
	    MPI_Send(buf, msg_len, MPI_CHAR, dest, 13, MPI_COMM_WORLD);
	    dest++;
	    if (dest % num_ranks == 0)   {
		dest= 1;
	    }
	}
	return 0.0;

    } else   {
	/* I'm a receiver */
	MPI_Recv(buf, msg_len, MPI_CHAR, 0, 13, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	t= MPI_Wtime();
	for (i= 1; i < num_msgs; i++)   {
	    MPI_Recv(buf, msg_len, MPI_CHAR, 0, 13, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	return MPI_Wtime() - t;
    }

}  /* end of Test2() */



static double
Test3(int my_rank, int num_ranks, int num_msgs, int msg_len)
{

double t;
int i;
int dest;


    if (my_rank == 0)   {
	/* I'm the receiver */
	t= MPI_Wtime();
	for (i= 0; i < num_msgs * (num_ranks - 1); i++)   {
	    MPI_Recv(buf, msg_len, MPI_CHAR, MPI_ANY_SOURCE, 14, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	return MPI_Wtime() - t;
    } else   {
	/* I'm a sender */
	dest= 0;
	for (i= 0; i < num_msgs; i++)   {
	    MPI_Send(buf, msg_len, MPI_CHAR, dest, 14, MPI_COMM_WORLD);
	}
	return 0.0;
    }

}  /* end of Test3() */



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-l len] [-n num]\n", pname);
    fprintf(stderr, "    -l len      Message length. Default %d\n", DEFAULT_MSG_LEN);
    fprintf(stderr, "    -n num      Number of messages per test. Default %d\n", DEFAULT_NUM_MSGS);

}  /* end of usage() */
