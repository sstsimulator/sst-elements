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
#include <assert.h>	/* For assert() */
#include <mpi.h>
#include "stat_p.h"

#undef TRUE
#define TRUE            (1)
#undef FALSE
#define FALSE           (0)

#define SIZE		(10485760)
#define READY		(10)
#define BCAST		(11)
#define LATENCY		(12)
#define DONE		(13)

void doit(int len, int trial, double *latency, int *msgs);
double aligned_buf[SIZE/sizeof(double)];
char *buf;
int my_rank, num_nodes;



int
main(int argc, char *argv[])
{

extern char *optarg;
int ch, error;

int len, start_len, end_len, inc, trials, i;
int user_inc;
int msgs;
int stat_mode;
double latency;
double tot_latency;
double tot_squared_latency;
double max_latency;
double min_latency;

double precision;


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
    error= FALSE;
    start_len= 0;
    end_len= 1048576;
    inc= 8; /* this inc will be set dynamically */
    user_inc= -1;
    trials= 100;
    stat_mode = FALSE;

    /* check command line args */
    while ((ch= getopt(argc, argv, "i:e:s:n:t")) != EOF)   {
	switch (ch)   {
	    case 'i':
		user_inc= strtol(optarg, (char **)NULL, 0);
		break;
	    case 'e':
		end_len= strtol(optarg, (char **)NULL, 0);
		if (end_len > SIZE)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Maximum end length is %d\n", SIZE);
		    }
		    error= 1;
		}
		break;
	    case 's':
		start_len= strtol(optarg, (char **)NULL, 0);
		break;
	    case 'n':
		trials= strtol(optarg, (char **)NULL, 0);
		break;
	    case 't': 
		stat_mode= TRUE;
		break;
	    default:
		error= TRUE;
		break;
	}
    }

    if (error)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Usage: %s [-s start_length] [-e end_length] [-i inc] [-n trials] [-m]\n", argv[0]);
	}
	exit (-1);
    }

    if (my_rank == 0)   {
	printf("# \n");
	if (user_inc > 0)   {
	    printf("# Results for %d trials each of length %d through %d in increments of %d\n#\n", 
		trials, start_len, end_len, user_inc);
	} else   {
	    printf("# Results for %d trials each of length %d through %d increasing increments\n#\n", 
		trials, start_len, end_len);
	}
	printf("# Exchanging data between nodes %d and %d.\n", 0, num_nodes - 1);
	printf("# Length                  Latency\n");
	printf("# in bytes            in micro seconds\n");
	printf("#           minimum     average     maximum     msgs per #trials       precision\n");
	printf("#                                               trial\n");
    }

    /* Touch the memory we are going to use */
    for (i= 0; i < (SIZE / sizeof(double)); i++)   {
	aligned_buf[i]= i;
    }

    len= start_len;

    while (len <= end_len)   {
	buf= (char *)aligned_buf;
	latency= 0.0;
	tot_latency= 0.0;
	tot_squared_latency= 0.0;
	max_latency= 0.0;
	min_latency= 1000000000.0;
	i= 0;
	
	if (stat_mode) {
	    trials= 10000;
	} else   {
	    trials= 100;
	}

	while (i < trials)   {
	    doit(len, i, &latency, &msgs);
	    assert(latency >= 0);
	    tot_latency= tot_latency + latency;
	    tot_squared_latency= tot_squared_latency + latency * latency;
	    if (latency < min_latency)   {
		min_latency= latency;
	    }
	    if (latency > max_latency)   {
		max_latency= latency;
	    }
	    
	    precision= stat_p(my_rank, i + 1, tot_latency, tot_squared_latency, latency);
	    
	    if (stat_mode)   {
		/* check for precision if at least 9 trials have taken place. i > 1 => N > 2. */
		if (my_rank == 0 && i > 7 && precision <= 0.01)   {
		    MPI_Bcast(&i, 1, MPI_INT, 0, MPI_COMM_WORLD);
		    if (0)   {
			printf("Node %d: Reached enough accuracy for msg size %d. Moving on to the next msg size.\n",
			    my_rank, len);
		    }
		    trials= i;
		} else   {
		    MPI_Bcast(&trials, 1, MPI_INT, 0, MPI_COMM_WORLD);
		}
	    }
	    i++;
	}

	MPI_Barrier(MPI_COMM_WORLD);

	if (stat_mode)   {
	    /* if i = 10, actual trials done is 11 */
	    trials++; /* this is the total number of trials that took place */
	}

	if (my_rank == 0)   {
	    printf("%9d  %8.2f    %8.2f    %8.2f    %3d      %5d        %6.2f\n",
		len, min_latency, tot_latency / trials, max_latency, msgs, trials, precision);
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



/*
** Sender pre-posts a receive
** Sender waits for ACK from receiver
** Sender does a ready send
** Sender releases all ranks
** Receiver pre-posts a receive
** Receiver sends ACK to sender
** Receiver waits for data
** Receiver sends data back (using ready send)
** Everybody waits for go on from sender
*/
#define MAX_PING_PONG	(25)
void
doit(int len, int trial, double *latency, int *msgs)
{

static double last_delta= 0.0;
static int pp= MAX_PING_PONG;
double delta, start;
int rc;
int j;


    MPI_Request latencyflag[MAX_PING_PONG];
    rc= 0;


    /*
    ** To make sure we have an acceptable timer resolution, we do multiple
    ** ping-pongs for smaller messages. We evaluate how many messages to
    ** send per trial at the beginning of a series.
    */
    if (trial == 0)   {
	if (last_delta > 0.001000)   {
	    pp--;
	}

	if (pp < 3)   {
	    pp= 3;
	}
    }

    *msgs= pp;
    if (my_rank == 0)   {
	/* Pre-post receive(s) */
	for (j= 0; j < pp; j++)   {
	    rc |= MPI_Irecv(buf, len, MPI_BYTE, num_nodes - 1, LATENCY, MPI_COMM_WORLD, &latencyflag[j]);
	}

	/* Wait for ACK from other node */
	rc |= MPI_Recv(buf, 0, MPI_BYTE, num_nodes - 1, READY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	start= MPI_Wtime();
	for (j= 0; j < pp; j++)   {
	    rc |= MPI_Rsend(buf, len, MPI_BYTE, num_nodes - 1, LATENCY, MPI_COMM_WORLD);
	    rc |= MPI_Wait(&latencyflag[j], MPI_STATUS_IGNORE);
	}
	delta = MPI_Wtime() - start;
	*latency= delta / pp * 1000000.0 / 2.0;
	last_delta= delta;

	MPI_Bcast(&last_delta, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    } else if (my_rank == num_nodes - 1)   {
	/* Run on the last node */
	/* Pre-post receive(s) */ 
	for (j= 0; j < pp; j++)   {
	    rc |= MPI_Irecv(buf, len, MPI_BYTE, 0, LATENCY, MPI_COMM_WORLD, &latencyflag[j]);
	}

	/* Send ACK */
	rc |= MPI_Send(buf, 0, MPI_BYTE, 0, READY, MPI_COMM_WORLD);

	for (j= 0; j < pp; j++)   {
	    rc |= MPI_Wait(&latencyflag[j], MPI_STATUS_IGNORE);
	    rc |= MPI_Rsend(buf, len, MPI_BYTE, 0, LATENCY, MPI_COMM_WORLD);
	}

	MPI_Bcast(&delta, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	*latency= delta / pp * 1000000.0 / 2.0;

    } else   {
	/* We're not involved, just participate in barrier */
	MPI_Bcast(&delta, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	*latency= delta / pp * 1000000.0 / 2.0;
    }

}  /* end of doit() */
