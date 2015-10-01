/*
** Copyright (c) 2011, IBM Corporation
** All rights reserved.
**
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
**
** A work stealing benchmark
**
*/
#include <stdio.h>
#include <stdlib.h>	/* For strtol(), exit() */
#include <stdint.h>	/* For uint64_t */
#include <unistd.h>	/* For getopt() */
#include <string.h>	/* For memset() */
#define __STDC_FORMAT_MACROS	(1)
#include <inttypes.h>	// For PRIu64
#include <math.h>	/* For log() */
#include <time.h>	/* For nanosleep() */
#include <errno.h>	/* For EINTR */
#include <assert.h>	/* For EINTR */
#include <mpi.h>
#include <list>

extern "C" {
#include "benchmarks/util.h"				// For disp_cmd_line() and DEFAULT_CMD_LINE_ERR_CHECK
}
#include "collective_patterns/collective_topology.h"	// For ctopo
#include "util/msg_counter.h"				// 
#include "benchmarks/Collectives/allreduce.h"		// For my_allreduce
#include "ws.h"


#define WS_VERSION			"0.3"
#define DEFAULT_TIME_STEP_DURATION	(500000) // In ns; i.e., 500 us
#define DEFAULT_ASK_TASKS		(4)	// How many ranks to ask for work, if we are out
#define MSG_SIZE_PER_WORK_UNIT		(3 * 1024)	// Migration cost depends on work size
#define DEFAULT_RANK_WORK_UNITS		(5000)	// Approximate number of work units per rank
#define DEFAULT_COMPLETION_CHECK	(25)	// Number of time steps between checks

#define TAG_WORK_REQUEST		(101)	// Tag for work request messages
#define TAG_WORK_MIGRATION		(105)	// Tag for work

/* Local functions */
static void usage(char *pname);
static float units2seconds(int duration);
static void clear_stats(void);
static char * alloc_work_buf(int size);
static int next_task_duration(void);
static int get_next_task(void);
static int assign_tasks(void);
static void work(void);
static void do_task_work(void);
static void handle_work_requests(char *buf, bool deny);
static void incoming_work(void);
static void clean_pending_sends(void);
bool static finish_task(int *current_task);
static void request_more_work(char *buf, int max_buf_size);
static void clean_pending_comm(char *buf);



/*
** Unfortunately these have to be (file) global to avoid massive
** argument lists to functions.
*/
static int verbose;
static double elapsed;
static uint64_t time_unit;
static std::list <int> *my_task_list;
static Msg_counter *counter;

static int debug;
static long int seed;
static int back_off;
static int circle;
static std::list <MPI_Request> *pending_sends;
static std::list <MPI_Request> *work_requests;
static int report_rank;
static int root;
static int rank_work_units;
static int completion_check;
static int time_step;

static int stat_time_worked;
static int stat_time_idle;
static int stat_tasks_completed;
static int stat_interrupts;
static int stat_tasks_left_in_queue;
static int stat_task_requests_sent;
static int stat_task_requests_not_granted;
static int stat_task_requests_rcvd;
static int stat_task_requests_denied;
static int stat_task_requests_granted;
static int stat_task_requests_honored;
static double stat_elapsed;
static int stat_initial_work_units;
static int stat_initial_tasks;
static int stat_global_initial_tasks;
static int stat_global_tasks_completed;
static int stat_gloabl_initial_work_units;
static int stat_global_time_worked;
static int stat_task_requests_attempted;
static uint64_t stat_total_sends;
static uint64_t stat_global_total_sends;



int
ws_init(int argc, char *argv[])
{

int ch;
bool error;
bool seed_set;


    if (num_ranks < 2)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Need to run on at least two ranks; more would be better\n");
	}
	return -1;
    }


    /* Set the defaults */
    opterr= 0;		/* Disable getopt error printing */
    error= false;
    verbose= 0;
    debug= 0;
    seed_set= false;
    time_unit= DEFAULT_TIME_STEP_DURATION;
    root= 0;
    report_rank= root;
    rank_work_units= DEFAULT_RANK_WORK_UNITS;
    completion_check= DEFAULT_COMPLETION_CHECK;
    counter= new Msg_counter(num_ranks);


    /* Check command line args */
    while ((ch= getopt(argc, argv, "dr:s:v")) != EOF)   {
        switch (ch)   {
	    case 'd':
		debug++;
		break;

            case 'r':
		report_rank= strtol(optarg, (char **)NULL, 0);
		if ((report_rank < 0) || (report_rank >= num_ranks))   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Report rank (-r %d) needs to be 0...%d\n",
			    report_rank, num_ranks - 1);
		    }
		    error= true;
		}
		break;

            case 's':
		seed= strtol(optarg, (char **)NULL, 0);
		seed_set= true;
		break;

            case 'v':
		verbose++;
		break;

	    /* Command line error checking */
	    DEFAULT_CMD_LINE_ERR_CHECK
        }
    }
 
    if (time_unit > 999999999)   {
	fprintf(stderr, "nanosleep() requires the nanosecond field to be less than 999999999.!\n");
	return -1;
    }

    if (!seed_set)   {
        if (my_rank == 0)   {
	    fprintf(stderr, "Seed (-s value) is a required argument\n");
	}
	error= true;
    }

    if (error)   {
        if (my_rank == 0)   {
            usage(argv[0]);
        }
	return -1;
    }


    if (my_rank == 0)   {
	printf("# Work stealing benchmark. Version %s\n", WS_VERSION);
	printf("# ------------------------------------\n");
	disp_cmd_line(argc, argv);
	printf("# Number of ranks:                                       %5d\n", num_ranks);
	printf("# Random number seed:                       %18ld\n", seed);
	printf("# One time step:                                  %12.9f s\n", units2seconds(1));
	printf("# Neighbors to contact in same time step when out of work  %3d (approx.)\n",
	    DEFAULT_ASK_TASKS);
	printf("# Message size per work unit during migration           %6d\n", MSG_SIZE_PER_WORK_UNIT);
	printf("# Amount of initial work assigned to each rank is 0 ...  %5d units\n", rank_work_units);
	printf("# Number of time steps between completion checks         %5d\n", completion_check);
    }


    if ((my_rank == 0) && (verbose))   {
	printf("Print something\n");
    }

    return 0;

}  /* end of ws_init() */



static void
clear_stats(void)
{

    stat_time_worked= 0;
    stat_time_idle= 0;
    stat_tasks_completed= 0;
    stat_interrupts= 0;
    stat_tasks_left_in_queue= 0;
    stat_task_requests_sent= 0;
    stat_task_requests_not_granted= 0;
    stat_task_requests_rcvd= 0;
    stat_task_requests_denied= 0;
    stat_task_requests_granted= 0;
    stat_task_requests_honored= 0;
    stat_elapsed= 0.0;
    stat_initial_work_units= 0;
    stat_initial_tasks= 0;
    stat_global_initial_tasks= 0;
    stat_global_tasks_completed= 0;
    stat_gloabl_initial_work_units= 0;
    stat_global_time_worked= 0;
    stat_task_requests_attempted= 0;
    stat_total_sends= 0;
    stat_global_total_sends= 0;

}  // end of clear_stats()



static char *
alloc_work_buf(int size)
{

char *work_buf;


    // Allocate a work buffer. We use it for sends and receives.
    // We don't use the data in it.
    work_buf= (char *)malloc(size);
    if (work_buf == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(-1);
    }
    memset(work_buf, 0, size);

    return work_buf;

}  // end of alloc_work_buf()



// Peek at the next task, but don't pop it off the list
static int
next_task_duration(void)
{

int duration= 0;


    if (!my_task_list->empty())   {
	duration= my_task_list->front();
    }
    return duration;

}  // end of next_task_duration()



static int
get_next_task(void)
{

int current_task= -1;


    if (!my_task_list->empty())   {
	current_task= my_task_list->front();
	my_task_list->pop_front();
    }
    return current_task;

}  // end of get_next_task()



// Initial assigned of tasks
static int
assign_tasks(void)
{

int duration;
int remainder;
int max_task_duration;


    max_task_duration= 0;

    // How much total work units do we want to do this time?
    stat_initial_work_units= drand48() * rank_work_units;
    remainder= stat_initial_work_units;

    while (remainder)   {
	// Break up the total work into tasks
	// May want to use a gamma distribution here at some point
	duration= drand48() * remainder;

	// Don't allow zero-length tasks
	if (duration < 1)   {
	    duration= 1;
	}

	my_task_list->push_back(duration);

	if (duration > max_task_duration)   {
	    max_task_duration= duration;
	}

	remainder= remainder - duration;
	stat_initial_tasks++;
    }

    // We need to know the largest possible task size that may get sent around
    // so we can allocate a buffer that is large enough.
    if (debug)   {
	printf("[%3d] Will start with %5d tasks totaling %8d work units, my largest work size is %d\n",
	    my_rank, stat_initial_tasks, stat_initial_work_units, max_task_duration);
    }

    MPI_Allreduce(MPI_IN_PLACE, &max_task_duration, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    if (debug && my_rank == 0)   {
	printf("      Max work size is %d\n", max_task_duration);
    }

    return max_task_duration * MSG_SIZE_PER_WORK_UNIT;

}  // end of assign_taks()



static void
work(void)
{

struct timespec ts;
struct timespec rem;
int rc;


    // nanosleep may come back early if interrupted, but it will tell
    // us how much time remains to "work". Send it back down if necessary
    ts.tv_sec= 0;
    ts.tv_nsec= time_unit;
 
    while ((rc= nanosleep(&ts, &rem)) != 0)   {
	if (rc == EINTR)   {
	    ts= rem;
	    stat_interrupts++;
	    // Continue the interrupted sleep
	} else   {
	    // Something was wrong...
	    break;
	}
    }

}  // end of work()



static void
do_task_work(void)
{

    // We don't actually do anything during work. Use nanosleep to pass
    // the time.
    work();


    // Lower our back off threshold. We use it to determine whether it
    // is OK now to ask for more work. (We come through here even if we
    // don't currently have work.)
    back_off--;
    if (back_off < 0)   {
	back_off= 0;
    }

}  // end of do_task_work()



static void
handle_work_requests(char *buf, bool deny)
{

int flag;
MPI_Status status;
MPI_Request req;
int send_off_task;


    // Check if we have any work requests
    while (true)   {
	flag= 0;
	MPI_Iprobe(MPI_ANY_SOURCE, TAG_WORK_REQUEST, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
	if (flag)   {
	    // Consume the request
	    MPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE, TAG_WORK_REQUEST, MPI_COMM_WORLD, &status);
	    counter->insert_log_entry(status.MPI_SOURCE, 0, time_step);
	    stat_task_requests_rcvd++;

	    // If my work queue is getting low, or my next task is short,
	    // don't give it away.
	    if (deny || (my_task_list->size() < 5) || (next_task_duration() < 2))   {
		// Sorry, no work for you
		MPI_Send(NULL, 0, MPI_BYTE, status.MPI_SOURCE, TAG_WORK_MIGRATION, MPI_COMM_WORLD);
		stat_task_requests_denied++;
		counter->record(status.MPI_SOURCE, 0);
		stat_total_sends++;

	    } else   {
		// Pass off some work
		send_off_task= get_next_task();
		MPI_Isend(buf, send_off_task * MSG_SIZE_PER_WORK_UNIT, MPI_BYTE,
		    status.MPI_SOURCE, TAG_WORK_MIGRATION, MPI_COMM_WORLD, &req);
		stat_task_requests_granted++;
		pending_sends->push_back(req);
		counter->record(status.MPI_SOURCE, send_off_task * MSG_SIZE_PER_WORK_UNIT);
		stat_total_sends++;
	    }

	} else   {
	    break;
	}
    }

}  // end of handle_work_requests()



static void
incoming_work(void)
{

int flag;
MPI_Status status;
MPI_Request req;
int count;
std::list <MPI_Request>::iterator it;


    // Check if we have received new work
    // Go through our request list and see if any have been answered
    for (it= work_requests->begin(); it != work_requests->end(); it++)   {
	flag= 0;
	req= *it;
	MPI_Test(&req, &flag, &status);
	if (flag)   {
	    // Got something. If msg size is larger than 0, it's size indicates
	    // how much work we got.
	    // If it is 0, we got a decline

	    MPI_Get_count(&status, MPI_BYTE, &count);
	    counter->insert_log_entry(status.MPI_SOURCE, count, time_step);
	    if (count > 0)   {
		// Add it to work list
		my_task_list->push_back(count / MSG_SIZE_PER_WORK_UNIT);
		stat_task_requests_honored++;
		circle= circle / 2;
		if (circle <= 1)   {
		    circle= 1;
		}

	    } else   {
		// If we get a decline, back off and increase the circle
		stat_task_requests_not_granted++;

		// Back off a little
		back_off= back_off + 5;

		// Widen the circle
		// This determines how far away from me I ask for work
		circle++;
	    }
	    it= work_requests->erase(it);
	}
    }

}  // end of incoming_work()



static void
clean_pending_sends(void)
{

int flag;
MPI_Request req;
std::list <MPI_Request>::iterator it;


    // Clean out any sends that have finished.
    // We use MPI_Isend but we don't really care when they finish, but
    // we have to clean them out once in a while to prevent memory
    // leaks and resource exhaustion
    for (it= pending_sends->begin(); it != pending_sends->end(); it++)   {
	flag= 0;
	req= *it;
	MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
	if (flag)   {
	    it= pending_sends->erase(it);
	}
    }

}  // end of clean_pending_sends()



// A task has completed. Start the next task if one is in the queue.
bool static
finish_task(int *current_task)
{

bool busy;


    // We are done with this task
    stat_tasks_completed++;


    if (my_task_list->empty())   {
	// We are out of work
	busy= false;
	if (debug > 0)   {
	    printf("[%3d] Out of work. Did %5d tasks %5d time units (%.9f s)\n",
		my_rank, stat_tasks_completed, stat_time_worked, units2seconds(stat_time_worked));
	}
    } else   {
	// Grab the next one to do
	*current_task= get_next_task();
	if (*current_task > 0)   {
	    busy= true;
	} else   {
	    busy= false;
	}
    }

    return busy;

}  // end of finish_task()



static void
request_more_work(char *buf, int max_buf_size)
{

int num_tasks;
MPI_Request req;
int dest;
int ask_tasks;


    // Send out some work requests to random nodes
    // Close ones initially, then farther away
    ask_tasks= DEFAULT_ASK_TASKS;
    num_tasks= (int)(-1.0 * log(drand48()) * ask_tasks);
    stat_task_requests_attempted++;
    for (int i= 0; i < num_tasks; i++)   {

	dest= (int)(drand48() * circle) % num_ranks;
	dest= dest - dest / 2;
	dest= (my_rank + dest + num_ranks) % num_ranks;
	assert(0 <= dest);
	assert(dest < num_ranks);

	if (dest == my_rank)   {
	    // No point in asking myself for more work
	    circle++;
	    continue;
	}

	// Post a work receive for the new task or the decline
	MPI_Irecv(buf, max_buf_size, MPI_BYTE, dest, TAG_WORK_MIGRATION, MPI_COMM_WORLD, &req);
	work_requests->push_back(req);

	// Send a work request
	MPI_Isend(NULL, 0, MPI_BYTE, dest, TAG_WORK_REQUEST, MPI_COMM_WORLD, &req);
	stat_task_requests_sent++;
	pending_sends->push_back(req);
	counter->record(dest, 0);
	stat_total_sends++;
    }

}  // end of request_more_work()



static void
clean_pending_comm(char *buf)
{

bool deny;


    // At the end, we may have pending sends that we
    // need to clean out of the system

    deny= true; // Don't send more work away
    while (!pending_sends->empty())   {
	clean_pending_sends();
	incoming_work();
	handle_work_requests(buf, deny);
    }

    // If we have pending work requests, wait for the NAC's
    while (!work_requests->empty())   {
	incoming_work();
	handle_work_requests(buf, deny);
    }

    // All our sends are out, and our work requests have been answered.
    // It is possible that more work requests are on the way to us
    // Hang around for a little while to NACK them
    // I'm not sure how long to make the loop below.
    for (int i= 0; i < 5; i++)   {
	work(); // wait a while
	handle_work_requests(buf, deny);
    }

}  // end of clean_pending_comm()




double
ws_work(void)
{

int current_task;
bool busy;
char *work_buf;
int max_buf_size;
bool deny;
double total_time_end;
double total_time_start;
bool done;
Collective_topology *ctopo;


    clear_stats();
    back_off= 0;
    circle= 1;
    done= false;
    ctopo= new Collective_topology(my_rank, num_ranks, TREE_DEEP);
    counter->clear();

    srand48(seed * (my_rank + 1));

    my_task_list= new std::list <int>;
    pending_sends= new std::list <MPI_Request>;
    work_requests= new std::list <MPI_Request>;

    max_buf_size= assign_tasks();
    work_buf= alloc_work_buf(max_buf_size);

    // Grab the first task on the list
    current_task= get_next_task();
    if (current_task > 0)   {
	busy= true;
    } else   {
	busy= false;
    }
    time_step= 0;


    // Sync and start timer
    MPI_Barrier(MPI_COMM_WORLD);
    total_time_start= MPI_Wtime();

    while (!done)   {
	do_task_work();
	deny= false; // Give work away, if we have enough
	handle_work_requests(work_buf, deny);
	incoming_work();
	clean_pending_sends();

	if (busy)   {
	    // I was working during do_task_work(), not just waiting
	    current_task--;
	    stat_time_worked++;

	    if (current_task <= 0)   {
		busy= finish_task(&current_task);
	    }
	} else   {
	    stat_time_idle++;

	    // Has new work arrived?
	    current_task= get_next_task();
	    if (current_task > 0)   {
		busy= true;
	    }
	}

	// If no longer busy, and we are not currently backing off,
	// then send requests for more work
	if (!busy && (back_off == 0) && work_requests->empty())   {
	    request_more_work(work_buf, max_buf_size);
	}

	time_step++;
	if ((time_step % completion_check) == 0)   {
	    // Time to check with everyone
	    double in;
	    double result;

	    in= busy;
	    my_allreduce(&in, &result, 1, ctopo);
	    if (result == 0.0)   {
		done= true;
	    }
	}
    }
    total_time_end= MPI_Wtime();

    clean_pending_comm(work_buf);

    free(work_buf);
    stat_tasks_left_in_queue= my_task_list->size();
    delete ctopo;
    delete my_task_list;
    delete pending_sends;
    delete work_requests;

    MPI_Reduce(&stat_tasks_completed, &stat_global_tasks_completed, 1, MPI_INT,
	MPI_SUM, root, MPI_COMM_WORLD);
    MPI_Reduce(&stat_time_worked, &stat_global_time_worked, 1, MPI_INT, MPI_SUM,
	root, MPI_COMM_WORLD);
    MPI_Reduce(&stat_initial_tasks, &stat_global_initial_tasks, 1, MPI_INT,
	MPI_SUM, root, MPI_COMM_WORLD);
    MPI_Reduce(&stat_initial_work_units, &stat_gloabl_initial_work_units, 1,
	MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);
    MPI_Reduce(&stat_total_sends, &stat_global_total_sends, 1, MPI_LONG_LONG_INT,
	MPI_SUM, root, MPI_COMM_WORLD);

    elapsed= total_time_end - total_time_start;
    MPI_Allreduce(&elapsed, &stat_elapsed, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    return stat_elapsed;

}  /* end of ws_work() */



void
ws_print(void)
{

int cnt;
uint64_t t;
const char *msg_cnt_fname= "ws_count.dat";
const char *msg_size_fname= "ws_size.dat";


    // Some consistency checking
    if (stat_task_requests_sent !=
	stat_task_requests_not_granted + stat_task_requests_honored)   {
	fprintf(stderr, "[%3d] Comm pending in system: requests %d, granted %d, denied %d\n",
		my_rank, stat_task_requests_sent, stat_task_requests_honored,
		stat_task_requests_not_granted);
    }

    // Tasks available to me
    cnt= stat_initial_tasks + stat_task_requests_honored;
    // Minus work done or given away
    cnt= cnt - stat_tasks_completed - stat_task_requests_granted;
    // Should equal what is left in queue
    if (cnt != stat_tasks_left_in_queue)   {
	fprintf(stderr, "[%3d] Work done and given away != work given + queue!\n", my_rank);
    }


    // Print some statistics
    if (my_rank == root)   {
	if (stat_global_initial_tasks != stat_global_tasks_completed)   {
	    fprintf(stderr, "      Total initial tasks %d != tasks completed %d\n",
		stat_global_tasks_completed, stat_global_tasks_completed);
	}
	if (stat_gloabl_initial_work_units != stat_global_time_worked)   {
	    fprintf(stderr, "      Total initial time units %d != time units completed %d\n",
		stat_gloabl_initial_work_units, stat_global_time_worked);
	}
	printf("#          Total tasks assigend        %6d\n", stat_global_initial_tasks);
	printf("#          Total tasks completed       %6d\n", stat_global_tasks_completed);
	printf("#          Total time units assigned   %6d\n", stat_gloabl_initial_work_units);
	printf("#          Total time units completed  %6d\n", stat_global_time_worked);
	printf("#          Total time         %12.6f seconds\n", stat_elapsed);
	printf("#          Total messages sent      %9" PRIu64 " (excluding allreduce)\n",
	    stat_global_total_sends);
    }


    if (my_rank == report_rank)   {
	printf("# Rank %3d My total time      %12.6f seconds\n", my_rank, elapsed);
	printf("# Rank %3d started with                %6d tasks,    %6d time units of work\n",
	    my_rank, stat_initial_tasks, stat_initial_work_units);
	printf("# Rank %3d completed                   %6d tasks,    %6d left in queue\n",
	    my_rank, stat_tasks_completed, stat_tasks_left_in_queue);
	printf("# Rank %3d worked                      %6d time units\n",
	    my_rank, stat_time_worked);
	printf("# Rank %3d was idle                    %6d time units\n",
	    my_rank, stat_time_idle);
	printf("# Rank %3d had                         %6d interrupts in nanosleep\n",
	    my_rank, stat_interrupts);
	printf("# Rank %3d received                    %6d task requests from other ranks\n",
	    my_rank, stat_task_requests_rcvd);
	printf("# Rank %3d denied                      %6d task requests to other ranks\n",
	    my_rank, stat_task_requests_denied);
	printf("# Rank %3d granted                     %6d task requests to other ranks\n",
	    my_rank, stat_task_requests_granted);
	printf("# Rank %3d attempted                   %6d times to ask for more work\n",
	    my_rank, stat_task_requests_attempted);
	printf("# Rank %3d sent                        %6d task requests to other ranks\n",
	    my_rank, stat_task_requests_sent);
	printf("# Rank %3d sent an average of          %6.2f task requests per round\n",
	    my_rank, (float)stat_task_requests_sent / stat_task_requests_attempted);
	printf("# Rank %3d had                         %6d task requests denied by other ranks\n",
	    my_rank, stat_task_requests_not_granted);
	printf("# Rank %3d had                         %6d task requests honored by other ranks\n",
	    my_rank, stat_task_requests_honored);
	printf("# Rank %3d sent                        %6" PRIu64 " messages (excluding allreduce)\n",
	    my_rank, stat_total_sends);
	printf("# Rank %3d back_off now                %6d, circle now %6d\n",
	    my_rank, back_off, circle);
    }

    t= counter->total_cnt();
    if (t != stat_total_sends)   {
	fprintf(stderr, "[%3d] Account problem in number of messages sent! %"
	    PRIu64 " != %" PRIu64 "\n", my_rank, t, stat_total_sends);
    }

    // Print the message count table suitable for gnuplot
    // in ascending order of rank
    if (my_rank == 0)   {
	unlink(msg_cnt_fname);
	unlink(msg_size_fname);
    }
    for (int i= 0; i < num_ranks; i++)   {
	if (my_rank == i)   {
	    counter->output_cnt(msg_cnt_fname);
	    counter->output_bytes(msg_size_fname);
	}
	MPI_Barrier(MPI_COMM_WORLD);
    }

    if (my_rank == 0)   {
	counter->show_log("Rank0trace.out");
    }

    delete counter;

}  /* end of ws_print() */



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s -s seed [-v] [-u unit] [-r rank]\n", pname);
    fprintf(stderr, "    -v               Increase verbosity\n");
    fprintf(stderr, "    -s seed          Random number seed to use.\n");
    fprintf(stderr, "    -u unit          Duration of one time unit in ns. Default %d\n",
	DEFAULT_TIME_STEP_DURATION);
    fprintf(stderr, "    -d               Debug\n");
    fprintf(stderr, "    -r rank          Rank that does reporting at end. Default %d\n", 0);

}  /* end of usage() */



// Convert time units to seconds
static float
units2seconds(int duration)
{

    return (duration * time_unit) / 1000000000.0;

}  // units2seconds()
