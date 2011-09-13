/*
**  $Id: mpilatency.c,v 1.2 2006-12-15 18:22:55 rolf Exp $
**
**  This is a pingpong test used to calculate 
**  latency and bandwidth for various message 
**  sizes.
**
*/
#include <stdio.h>
#include <stdlib.h>	/* For strtol(), exit() */
#include <unistd.h>	/* For getopt() */
#include "mpi.h"

#undef TRUE
#define TRUE            (1)
#undef FALSE
#define FALSE           (0)

#define SIZE		(10485760)
#define READY		(10)
#define BCAST		(11)
#define LATENCY		(12)
#define DONE		(13)

void doit(int len, int trial, double *latency, double *bandwidth, int *msgs);
double aligned_buf[SIZE/sizeof(double)];
char *buf;
int my_node, num_nodes;

int
main(int argc, char *argv[])
{

extern char *optarg;
int ch, error;

int len, start_len, end_len, inc, trials, i;
int user_inc;
int msgs;
int mega;
double latency, bandwidth;
double tot_latency, tot_bandwidth;
double max_latency, max_bandwidth;
double min_latency, min_bandwidth;



    /* MPI_Init((int *)&argc, (char ***)&argv); */
    MPI_Init(&argc, &argv);
 
    MPI_Comm_rank(MPI_COMM_WORLD, &my_node);
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);

    if (num_nodes < 2)   {
	if (my_node == 0)   {
	    fprintf(stderr, "Need to run on at least two nodes\n");
	}
	exit(-1);
    }


    /* Set the defaults */
    error= FALSE;
    start_len= 0;
    end_len= 1048576;
    inc= 8;
    user_inc= -1;
    trials= 100;
    mega= FALSE;
 
    /* check command line args */
    while ((ch= getopt(argc, argv, "i:e:s:n:m")) != EOF)   {
        switch (ch)   {
            case 'i':
		user_inc= strtol(optarg, (char **)NULL, 0);
		break;
            case 'e':
		end_len= strtol(optarg, (char **)NULL, 0);
		if (end_len > SIZE)   {
		    if (my_node == 0)   {
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
	    case 'm': 
		mega= TRUE;
		break;
            default:
		error= TRUE;
		break;
        }
    }
 
    if (error)   {
        if (my_node == 0)   {
            fprintf(stderr, "Usage: %s [-s start_length] [-e end_length] [-i inc] [-n trials] [-m]\n", argv[0]);
        }
        exit (-1);
    }



    if (my_node == 0)   {
	printf("# \n");
	if (user_inc > 0)   {
	    printf("# Results for %d trials each of length %d through %d in increments of %d\n#\n", 
		trials, start_len, end_len, user_inc);
	} else   {
	    printf("# Results for %d trials each of length %d through %d increasing increments\n#\n", 
		trials, start_len, end_len);
	}
	printf("# Exchanging data between nodes %d and %d.\n", 0, num_nodes - 1);
	printf("# Length                  Latency                             Bandwidth\n");
	printf("# in bytes            in micro seconds                ");
	if (mega)   {
	    printf("# in mega bytes per second\n");
	} else   {
	    printf("# in million bytes per second\n");
	}
	printf("#           minimum     average     maximum     minimum     average     maximum   msgs per trial\n");
    }

    /* Touch the memory we are going to use */
    for (i= 0; i < (SIZE / sizeof(double)); i++)   {
	aligned_buf[i]= i;
    }



    len= start_len;
    while (len <= end_len)   {
	buf= (char *)aligned_buf;
	latency= tot_latency= 0.0;
	max_latency= 0.0;
	min_latency= 1000000000.0;
	bandwidth= tot_bandwidth= 0.0;
	max_bandwidth= 0.0;
	min_bandwidth= 1000000000.0;

	for (i= 0; i < trials; i++)   {
	    doit(len, i, &latency, &bandwidth, &msgs);
	    tot_latency= tot_latency + latency;
	    if (latency < min_latency)   {
		min_latency= latency;
	    }
	    if (latency > max_latency)   {
		max_latency= latency;
	    }
	    tot_bandwidth= tot_bandwidth + bandwidth;
	    if (bandwidth < min_bandwidth)   {
		min_bandwidth= bandwidth;
	    }
	    if (bandwidth > max_bandwidth)   {
		max_bandwidth= bandwidth;
	    }
	}

	if (my_node == 0)   {
	    printf("%9d  %8.2f    %8.2f    %8.2f    ",
		len, min_latency, tot_latency / trials, max_latency);
	    if (mega)   {
		printf("%8.2f    %8.2f    %8.2f", 
		    min_bandwidth / (1024 * 1024),
		    (tot_bandwidth / trials) / (1024 * 1024), 
		    max_bandwidth / (1024 * 1024));
	    } else   {
		printf("%8.2f    %8.2f    %8.2f", 
		    min_bandwidth / 1000000.0, 
		    (tot_bandwidth / trials) / 1000000.0, 
		    max_bandwidth / 1000000.0);
	    }
	    printf("    # %3d\n", msgs);
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
doit(int len, int trial, double *latency, double *bandwidth, int *msgs)
{

static double last_delta= 0.0;
static int pp= MAX_PING_PONG;
MPI_Request latencyflag[MAX_PING_PONG];

double delta, start;
int rc = 0;
int j;


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
    if (my_node == 0)   {
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
	if (delta != 0.0)    {
	  *bandwidth= len / (delta) * 2.0;
        } else    {
	  *bandwidth = 0;
	}

	MPI_Bcast(&last_delta, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

    } else if (my_node == num_nodes - 1)   {
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

	MPI_Bcast(&last_delta, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

    } else   {
	/* We're not involved, just participate in barrier */
	MPI_Bcast(&last_delta, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
    }

}  /* end of doit() */
