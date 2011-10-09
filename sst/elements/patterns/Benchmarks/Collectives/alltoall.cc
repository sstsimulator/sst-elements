#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <mpi.h>
#include "alltoall.h"

// Instead of using the MPI provided alltoall, we use this algorithm.
// It is the same one used in the communication pattern and allows for
// more accurate comparisons, since the number of messages as well as
// sources and destinations are the same.



static bool is_pow2(int num);
static uint32_t next_power2(uint32_t v);



void
my_alltoall(double *in, double *result, int msg_len, int nranks, int my_rank)
{

int i;
int shift, dest;
int src;
int offset;
int len1, len2;
MPI_Request sreq1, sreq2;
MPI_Request rreq1, rreq2;
int64_t bytes_sent= 0;


    /* My own contribution */
    for (i= 0; i < msg_len; i++)   {
	result[my_rank * msg_len + i]= in[my_rank * msg_len + i];
    }

    i= nranks >> 1;
    shift= 1;
    while (i > 0)   {
	dest= (my_rank + shift) % nranks;
	src= ((my_rank - shift) + nranks) % nranks;

	sreq1= MPI_REQUEST_NULL;
	sreq2= MPI_REQUEST_NULL;
	rreq1= MPI_REQUEST_NULL;
	rreq2= MPI_REQUEST_NULL;

	offset= (my_rank * msg_len) - ((shift - 1) * msg_len);
	if (offset < 0)   {
	    /* Need to break it up into two pieces */
	    offset= offset + (nranks * msg_len);
	    len1= (nranks * msg_len) - offset;
	    MPI_Isend(result + offset, len1, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD, &sreq1);
	    bytes_sent= bytes_sent + len1 * sizeof(double);
	    len2= shift * msg_len - (nranks * msg_len - offset);
	    MPI_Isend(result, len2, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD, &sreq2);
	    bytes_sent= bytes_sent + len2 * sizeof(double);
	} else   {
	    /* I can send it in one piece */
	    MPI_Isend(result + offset, shift * msg_len, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD, &sreq1);
	    bytes_sent= bytes_sent + shift * msg_len * sizeof(double);
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

    // If we are not on a power of two ranks, we have some more work to do.
    // n additional pieces of data (of length msg_len) still need to be xfered
    if (!is_pow2(nranks))   {
	int n;
	double *r1_start, *r2_start;
	double *s1_start, *s2_start;
	int r1_len, r2_len;
	int s1_len, s2_len;


	// One more step: Each rank sends n blocks of data n ranks back, if
	// we are n over the last power of two
	n= nranks - next_power2(nranks) / 2;
	// if (my_rank == 0)   fprintf(stderr, "nranks %d, next pow %d, n %d\n", nranks, next_power2(nranks), n);
	dest= (my_rank + nranks - n) % nranks;
	// is the same as dest= (my_rank + shift) % nranks;
	assert(dest == (my_rank + shift) % nranks);
	src= (my_rank + n + nranks) % nranks;

	sreq1= MPI_REQUEST_NULL;
	sreq2= MPI_REQUEST_NULL;
	rreq1= MPI_REQUEST_NULL;
	rreq2= MPI_REQUEST_NULL;

	// Do I need to receive in two pieces?
	if (src < n - 1)   {
	    // Receive it in two pieces
	    r1_start= &result[(my_rank + 1) * msg_len];
	    r1_len= (nranks - my_rank - 1) * msg_len;
	    r2_start= &result[0];
	    r2_len= n * msg_len - r1_len;;
	    // fprintf(stderr, "[%3d] expect two of lengths %d and %d from %d\n", my_rank, r1_len, r2_len, src);
	    MPI_Irecv(r1_start, r1_len, MPI_DOUBLE, src, 0, MPI_COMM_WORLD, &rreq1);
	    MPI_Irecv(r2_start, r2_len, MPI_DOUBLE, src, 0, MPI_COMM_WORLD, &rreq2);
	} else   {
	    // Receive it all in one piece
	    r1_start= &result[((my_rank + 1) % nranks) * msg_len];
	    r1_len= n * msg_len;
	    // fprintf(stderr, "[%3d] expect one from %d\n", my_rank, src);
	    MPI_Irecv(r1_start, r1_len, MPI_DOUBLE, src, 0, MPI_COMM_WORLD, &rreq1);
	}


	// Send one piece or two?
	if (my_rank < n - 1)   {
	    // Split buffer; send two pieces
	    s1_start= &result[(nranks - (n - my_rank - 1)) * msg_len];
	    s1_len= (n - my_rank - 1) * msg_len;
	    s2_start= &result[0];
	    s2_len= (my_rank + 1) * msg_len;
	    // fprintf(stderr, "[%3d] Sending two pieces (%4.1f, %4.1f) of lengths %d, %d to %d\n", my_rank, *s1_start, *s2_start, s1_len, s2_len, dest);
	    MPI_Isend(s1_start, s1_len, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD, &sreq1);
	    bytes_sent= bytes_sent + s1_len;
	    MPI_Isend(s2_start, s2_len, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD, &sreq2);
	    bytes_sent= bytes_sent + s2_len;
	} else   {
	    // All in one piece
	    s1_start= &result[(my_rank - n + 1) * msg_len];
	    s1_len= n * msg_len;
	    // fprintf(stderr, "[%3d] Sending one piece (%4.1f) of len %d to %d\n", my_rank, *s1_start, s1_len, dest);
	    MPI_Isend(s1_start, s1_len, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD, &sreq1);
	    bytes_sent= bytes_sent + s1_len;
	}

	/* Clean up */
	MPI_Wait(&rreq1, MPI_STATUS_IGNORE);
	// fprintf(stderr, "[%3d] Got piece 1 of len %d: %4.1f from %d\n", my_rank, r1_len, *r1_start, src);
	MPI_Wait(&sreq1, MPI_STATUS_IGNORE);
	// fprintf(stderr, "[%3d] Sent piece 1 of len %d: %4.1f to %d\n", my_rank, s1_len, *s1_start, dest);
	if (rreq2 != MPI_REQUEST_NULL)   {
	    MPI_Wait(&rreq2, MPI_STATUS_IGNORE);
	    // fprintf(stderr, "[%3d] Got piece 2 of len %d: %4.1f from %d\n", my_rank, r2_len, *r2_start, src);
	}
	if (sreq2 != MPI_REQUEST_NULL)   {
	    MPI_Wait(&sreq2, MPI_STATUS_IGNORE);
	    // fprintf(stderr, "[%3d] Sent piece 2 of len %d: %4.1f to %d\n", my_rank, s2_len, *s2_start, dest);
	}
    }

    /* fprintf(stderr, "[%3d] bytes sent is %lld\n", my_rank, bytes_sent); */

}  /* end of my_alltoall() */



// from http://graphics.stanford.edu/~seander/bithacks.html
static bool
is_pow2(int num)
{
    if (num < 1)   {
	return false;
    }

    if ((num & (num - 1)) == 0)   {
	return true;
    }

    return false;

}  /* end of is_pow2() */



static uint32_t
next_power2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;

}  // end of next_power2()
