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

#define DEFAULT_MSG_LEN		(1024)
#define MSG_OPS			(100)

#define TAG_READY		(20)
#define TAG_LATENCY		(21)
#define TAG_DONE		(22)

#define	MAX_NUM_PREPOST	(16)

static void experiment(int my_rank, int max_trials, int num_ops, int msg_len, int dest,
    double req_precision, double (*do_one_trial)(int num_ops, int msg_len, int dest));
static double do_one_Test2_trial(int num_ops, int msg_len, int dest);
void doit(int len, int trial, double *latency, int *msgs);
double aligned_buf[DEFAULT_MSG_LEN/sizeof(double)];
char *buf;
int my_rank, num_nodes;



int
main(int argc, char *argv[])
{

extern char *optarg;
int ch, error;

int len;
int max_trials;
int stat_mode;
double req_precision;
int num_ops;
int dest;


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
    len= DEFAULT_MSG_LEN;
    stat_mode= TRUE;
    req_precision= DEFAULT_PRECISION;
    dest= num_nodes - 1;


    /* check command line args */
    while ((ch= getopt(argc, argv, "l:p:")) != EOF)   {
	switch (ch)   {
	    case 'l':
		len= strtol(optarg, (char **)NULL, 0);
		if (len < 0)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "# length (-l %d) must be >= 0\n", len);
		    }
		    error= TRUE;
		}
		break;

            case 'p':
		req_precision= strtod(optarg, (char **)NULL);
		if (req_precision == 0.0)   {
		    stat_mode= FALSE;
		}
		break;

	    /* Command line error checking */
	    DEFAULT_CMD_LINE_ERR_CHECK
	}
    }

    if (error)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Usage: %s [-l message length] [-p precision]\n", argv[0]);
	}
	exit (-1);
    }

    if (stat_mode)   {
	max_trials= MAX_TRIALS;
    } else   {
	max_trials= 1;
    }

    if (my_rank == 0)   {
	printf("# Router characterization benchmark\n");
	printf("# ---------------------------------\n");
	disp_cmd_line(argc, argv);
	printf("#\n");
	printf("# Running experiments with messages of length %d\n", len);
	if (stat_mode)   {
	    printf("# Requested precision is %.3f%%\n", req_precision * 100.0);
	} else   {
	    printf("# Statistics mode is off\n");
	}
	printf("#\n");
	printf("# Number of               Time\n");
	printf("# messages            in micro seconds\n");
	printf("#            minimum         mean       median      maximum           sd       precision  trials\n");
    }

    num_ops= 1;
    while (num_ops <= MSG_OPS)   {
	experiment(my_rank, max_trials, num_ops, len, dest, req_precision,
	    &do_one_Test2_trial);
	num_ops++;
    }

    MPI_Finalize();

    return 0;

}  /* end of main() */



static double
do_one_Test2_trial(int num_ops, int msg_len, int dest)
{

int j;
double t, t2;
MPI_Request latencyflag[MSG_OPS];
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

    return t2;

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

	/* Need at least tree trials */
	if ((cnt >= 3) && (precision < req_precision))   {
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
	printf("%9d  ", num_ops);
	disp_stats(results, cnt, precision);
	printf("   %5d\n", cnt);
    }

    free(results);

}  /* end of experiment() */
