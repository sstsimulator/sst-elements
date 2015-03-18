// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
**  $Id: mpilatency.c,v 1.2 2006-12-15 18:22:55 rolf Exp $
**
**  This is a pingpong test used to calculate 
**  end-to-end time for increasing message lengths.
**
*/
#include <stdio.h>
#include <stdlib.h>	/* For strtol(), exit() */
#include <unistd.h>	/* For getopt() */
#include <string.h>	/* For memset() */
#include <assert.h>	/* For assert() */
#include <mpi.h>
#include "stat_p.h"
#include "util.h"

#undef TRUE
#define TRUE            	(1)
#undef FALSE
#define FALSE           	(0)

#define DEFAULT_PRECISION	(0.01)
#define MAX_TRIALS		(20000)

#define MAX_MSG_LEN		(10485760)
#define TAG_READY		(20)
#define TAG_LATENCY		(21)
#define TAG_DONE		(22)

#define SHORT_MSG_OPS		(500)
#define MEDIUM_MSG_OPS		(50)
#define LONG_MSG_OPS		(5)
#define SHORT_MEDIUM_CUTOFF	( 2 * 1024)
#define MEDIUM_LONG_CUTOFF	(16 * 1024)

#define	MAX_NUM_PREPOST	(16)

static void experiment(int my_rank, int max_trials, int num_ops, int msg_len, int dest,
    double req_precision, double (*do_one_trial)(int num_ops, int msg_len, int dest));
static double do_one_Test1_trial(int num_ops, int msg_len, int dest);
static double do_one_Test2_trial(int num_ops, int msg_len, int dest);
void doit(int len, int trial, double *latency, int *msgs);
double aligned_buf[MAX_MSG_LEN/sizeof(double)];
char *buf;
int my_rank, num_nodes;



int
main(int argc, char *argv[])
{

extern char *optarg;
int ch, error;

int len;
int start_len;
int end_len;
int inc;
int max_trials;
int user_inc;
int stat_mode;
double req_precision;
int num_ops;
int dest;
int pingping;


    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);

    if (num_nodes < 2)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Need to run on at least two nodes\n");
	}
	exit(-1);
    }

    /* Set the defaults */
    opterr= 0;		/* Disable getopt error printing */
    error= FALSE;
    start_len= 0;
    end_len= MAX_MSG_LEN / 8;
    inc= 8; /* this inc will be set dynamically */
    user_inc= -1;
    stat_mode= TRUE;
    req_precision= DEFAULT_PRECISION;
    dest= num_nodes - 1;
    pingping= FALSE;


    /* check command line args */
    while ((ch= getopt(argc, argv, "ad:e:i:p:s:")) != EOF)   {
	switch (ch)   {
	    case 'a':
		pingping= TRUE;
		break;

	    case 'd':
		dest= strtol(optarg, (char **)NULL, 0);
		if ((dest < 0) || (dest >= num_nodes))   {
		    if (my_rank == 0)   {
			fprintf(stderr, "# destination (-d) must be 0 <= %d < n (%d)\n",
			    dest, num_nodes);
		    }
		    error= TRUE;
		}
		break;

	    case 'e':
		end_len= strtol(optarg, (char **)NULL, 0);
		if (end_len > MAX_MSG_LEN)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "# Maximum end length is %d\n", MAX_MSG_LEN);
		    }
		    error= TRUE;
		}
		break;

	    case 'i':
		user_inc= strtol(optarg, (char **)NULL, 0);
		break;

            case 'p':
		req_precision= strtod(optarg, (char **)NULL);
		if (req_precision == 0.0)   {
		    stat_mode= FALSE;
		}
		break;

	    case 's':
		start_len= strtol(optarg, (char **)NULL, 0);
		if (start_len < 0)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "# Minimum start length is %d\n", 0);
		    }
		    error= TRUE;
		}
		break;

	    /* Command line error checking */
	    DEFAULT_CMD_LINE_ERR_CHECK
	}
    }

    if (error)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Usage: %s [-a] [-d destination] [-s start_length] [-e end_length] [-i inc] [-p precision]\n",
		argv[0]);
	    fprintf(stderr, "           -a            Do ping-ping instead of ping-pong\n");
	    fprintf(stderr, "           -d            Destination rank. Default is n - 1\n");
	}
	exit (-1);
    }

    if (stat_mode)   {
	max_trials= MAX_TRIALS;
    } else   {
	max_trials= 1;
    }

    if (my_rank == 0)   {
	if (pingping)   {
	    printf("# Ping-Ping benchmark\n");
	} else   {
	    printf("# Ping-pong benchmark\n");
	}
	printf("# ------------------\n");
	disp_cmd_line(argc, argv);
	printf("#\n");
	if (user_inc > 0)   {
	    printf("# Running experiments of length %d through %d in increments of %d\n", 
		start_len, end_len, user_inc);
	} else   {
	    printf("# Running experiments of length %d through %d increasing increments\n", 
		start_len, end_len);
	}
	printf("# Exchanging data between nodes %d and %d of, %d nodes\n", 0, dest, num_nodes);
	if (stat_mode)   {
	    printf("# Requested precision is %.3f%%\n", req_precision * 100.0);
	} else   {
	    printf("# Statistics mode is off\n");
	}
	printf("#\n");
	printf("# Length                  Latency\n");
	printf("# in bytes            in micro seconds\n");
	printf("#            minimum         mean       median      maximum           sd       precision  trials\n");
    }

    len= start_len;

    while (len <= end_len)   {
	
	if (len < SHORT_MEDIUM_CUTOFF)   {
	    num_ops= SHORT_MSG_OPS;
	} else if (len < MEDIUM_LONG_CUTOFF)   {
	    num_ops= MEDIUM_MSG_OPS;
	} else   {
	    num_ops= LONG_MSG_OPS;
	}

	if (pingping)   {
	    experiment(my_rank, max_trials, num_ops, len, dest, req_precision,
		&do_one_Test2_trial);
	} else   {
	    experiment(my_rank, max_trials, num_ops, len, dest, req_precision,
		&do_one_Test1_trial);
	}

	if (user_inc <= 0)   {
	    if (len >= (inc << 4))   {
		inc= inc << 1;
	    }
	    len= len + inc;
	} else   {
	    len= len + user_inc;
	}
    }

    MPI_Finalize();

    return 0;

}  /* end of main() */



static double
do_one_Test1_trial(int num_ops, int msg_len, int dest)
{

int j;
double t, t2;
MPI_Request latencyflag[SHORT_MSG_OPS];


    /* Touch the memory we are going to use */
    memset(aligned_buf, 0xAA, msg_len);
    memset(aligned_buf, 0x55, msg_len);

    /* Do a warm-up */
    j= 0;
    if (my_rank == 0)   {
	MPI_Irecv(aligned_buf, msg_len, MPI_BYTE, dest, TAG_LATENCY, MPI_COMM_WORLD, &latencyflag[j]);
	MPI_Recv(NULL, 0, MPI_BYTE, dest, TAG_READY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Rsend(aligned_buf, msg_len, MPI_BYTE, dest, TAG_LATENCY, MPI_COMM_WORLD);
	MPI_Wait(&latencyflag[j], MPI_STATUS_IGNORE);

    } else if (my_rank == dest)   {
	MPI_Irecv(aligned_buf, msg_len, MPI_BYTE, 0, TAG_LATENCY, MPI_COMM_WORLD, &latencyflag[j]);
	MPI_Send(NULL, 0, MPI_BYTE, 0, TAG_READY, MPI_COMM_WORLD);
	MPI_Wait(&latencyflag[j], MPI_STATUS_IGNORE);
	MPI_Rsend(aligned_buf, msg_len, MPI_BYTE, 0, TAG_LATENCY, MPI_COMM_WORLD);
    }



    /* Post warm-up: Run the actual experiment */
    t2= 0.0;
    if (my_rank == 0)   {
	/* Pre-post receive(s) */
	for (j= 0; j < num_ops; j++)   {
	    MPI_Irecv(aligned_buf, msg_len, MPI_BYTE, dest, TAG_LATENCY, MPI_COMM_WORLD, &latencyflag[j]);
	}

	/* Wait for ACK from other node */
	MPI_Recv(NULL, 0, MPI_BYTE, dest, TAG_READY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	/* Start the timer */
	t= MPI_Wtime();
	for (j= 0; j < num_ops; j++)   {
	    MPI_Rsend(aligned_buf, msg_len, MPI_BYTE, dest, TAG_LATENCY, MPI_COMM_WORLD);
	    MPI_Wait(&latencyflag[j], MPI_STATUS_IGNORE);
	}
	t2= MPI_Wtime() - t;

    } else if (my_rank == dest)   {
	/* Pre-post receive(s) */ 
	for (j= 0; j < num_ops; j++)   {
	    MPI_Irecv(aligned_buf, msg_len, MPI_BYTE, 0, TAG_LATENCY, MPI_COMM_WORLD, &latencyflag[j]);
	}

	/* Send ACK */
	MPI_Send(NULL, 0, MPI_BYTE, 0, TAG_READY, MPI_COMM_WORLD);

	for (j= 0; j < num_ops; j++)   {
	    MPI_Wait(&latencyflag[j], MPI_STATUS_IGNORE);
	    MPI_Rsend(aligned_buf, msg_len, MPI_BYTE, 0, TAG_LATENCY, MPI_COMM_WORLD);
	}
    }

    return t2 / num_ops / 2.0;

}  /* end of do_one_Test1_trial() */



static double
do_one_Test2_trial(int num_ops, int msg_len, int dest)
{

int j;
double t, t2;
MPI_Request latencyflag[SHORT_MSG_OPS];
int prepost;


    /* Touch the memory we are going to use */
    memset(aligned_buf, 0xAA, msg_len);
    memset(aligned_buf, 0x55, msg_len);

    /* Do a warm-up */



    /* Post warm-up: Run the actual experiment */
    t2= 0.0;
    if (my_rank == 0)   {
	/* Wait for ACK from other node */
	MPI_Recv(NULL, 0, MPI_BYTE, dest, TAG_READY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	/* Start the timer */
	t= MPI_Wtime();
	for (j= 0; j < num_ops; j++)   {
	    MPI_Isend(aligned_buf, msg_len, MPI_BYTE, dest, TAG_LATENCY, MPI_COMM_WORLD, &latencyflag[j]);
	}
	MPI_Recv(NULL, 0, MPI_BYTE, dest, TAG_DONE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	t2= MPI_Wtime() - t;

	/* Clean up outside the timing loop */
	for (j= 0; j < num_ops; j++)   {
	    MPI_Wait(&latencyflag[j], MPI_STATUS_IGNORE);
	}

    } else if (my_rank == dest)   {
	/* Pre-post MAX_NUM_PREPOST receives */ 
	if (num_ops >= MAX_NUM_PREPOST)   {
	    prepost= MAX_NUM_PREPOST;
	} else   {
	    prepost= num_ops;
	}
	for (j= 0; j < prepost; j++)   {
	    MPI_Irecv(aligned_buf, msg_len, MPI_BYTE, 0, TAG_LATENCY, MPI_COMM_WORLD, &latencyflag[j]);
	}

	/* Send ACK */
	MPI_Send(NULL, 0, MPI_BYTE, 0, TAG_READY, MPI_COMM_WORLD);

	for (j= 0; j < num_ops; j++)   {
	    MPI_Wait(&latencyflag[j], MPI_STATUS_IGNORE);
	    if (j + prepost < num_ops)   {
		/* Post another */
		MPI_Irecv(aligned_buf, msg_len, MPI_BYTE, 0, TAG_LATENCY, MPI_COMM_WORLD,
		    &latencyflag[j + prepost]);
	    }
	}

	/* Got them all */
	MPI_Send(NULL, 0, MPI_BYTE, 0, TAG_DONE, MPI_COMM_WORLD);
    }

    return t2 / num_ops / 2.0;

}  /* end of do_one_Test2_trial() */



static void
experiment(int my_rank, int max_trials, int num_ops, int msg_len, int dest,
    double req_precision, double (*do_one_trial)(int num_ops, int msg_len, int dest))
{

double tot;
double tot_squared;
double metric;
int cnt;
double precision;
int done;
double *results;


    tot= 0.0;
    cnt= 0;
    tot_squared= 0.0;
    precision= 999.9;

    results= (double *)malloc(sizeof(double) * max_trials);
    if (results == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(-1);
    }

    while (cnt < max_trials)   {
	cnt++;

	/* Run one trial */
	metric= do_one_trial(num_ops, msg_len, dest);
	results[cnt - 1]= metric * 1000000.0;

	tot= tot + metric;
	tot_squared= tot_squared + metric * metric;
	precision= stat_p(cnt, tot, tot_squared);

	/* Need at least nine trials */
	if ((cnt >= 9) && (precision < req_precision))   {
	    /* Attained the required precisions. Done! */
	    done= 1;
	} else   {
	    done= 0;
	}

	/* 0 tells everyone else */
	MPI_Bcast(&done, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if (done)   {
	    break;
	}
    }

    if (my_rank == 0)   {
	printf("%9d  ", msg_len);
	disp_stats(results, cnt, precision);
	printf("   %5d\n", cnt);
    }

    free(results);

}  /* end of experiment() */
