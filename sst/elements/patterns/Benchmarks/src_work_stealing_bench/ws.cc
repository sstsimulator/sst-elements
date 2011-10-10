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
#include <math.h>	/* For log() */
#include <time.h>	/* For nanosleep() */
#include <errno.h>	/* For EINTR */
#include <assert.h>	/* For EINTR */
#include <mpi.h>
#include <list>

#include "util.h"	/* For disp_cmd_line() and DEFAULT_CMD_LINE_ERR_CHECK */
#include "ws.h"

#define WS_VERSION			"0.2"
#define DEFAULT_SEED			(543219876)
#define DEFAULT_TIME_STEPS		(360000)
#define DEFAULT_TIME_STEP_DURATION	(500000) // In ns; i.e., 500 us
#define DEFAULT_INITIAL_TASKS		(10)
#define DEFAULT_TASK_DURATION		(150)	// in time steps
#define MAX_TASK_DURATION		(450)	// in time steps
#define DEFAULT_NUM_NEW_TASKS		(2)	// Average of new tasks, if a tasks spawns others
#define DEFAULT_ASK_NEW_TASKS		(4)	// How many ranks to ask for work, if we are out
#define MSG_SIZE_PER_WORK_UNIT		(3 * 1024)	// Migration cost depends on work size

#define TAG_WORK_REQUEST		(101)	// Tag for work request messages
#define TAG_WORK_MIGRATION		(105)	// Tag for work

/* Local functions */
static void usage(char *pname);
static float units2seconds(int duration);
static int gen_new_task(void);



/*
** Unfortunately these have to be (file) global to avoid massive
** argument lists to functions.
*/
static int verbose;
static double elapsed;
static int time_steps;
static uint64_t time_unit;
static std::list <int> *my_task_list;

static int debug;
static int max_task_duration;
static int initial_tasks;
static long int seed;
static int new_tasks;
static int num_start_tasks;
static int back_off;
static int circle;
static double p_task_creation;		// Prob. of a new task being created when one finishes
static std::list <MPI_Request> *pending_sends;
static std::list <MPI_Request> *work_requests;
static int report_rank;
static int root;

static int stat_time_worked;
static int stat_time_idle;
static int stat_tasks_completed;
static int stat_interrupts;
static int stat_tasks_left_in_queue;
static int stat_initial_work_time;
static int stat_new_tasks;
static int stat_new_task_requests_sent;
static int stat_new_task_requests_not_granted;
static int stat_new_task_requests_rcvd;
static int stat_new_task_requests_denied;
static int stat_new_task_requests_granted;
static int stat_new_task_requests_honored;
static int stat_total_tasks;
static int stat_total_time_units;
static double stat_elapsed;



int
ws_init(int argc, char *argv[])
{

int ch;
bool error;


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
    seed= DEFAULT_SEED;
    time_steps= DEFAULT_TIME_STEPS;
    initial_tasks= DEFAULT_INITIAL_TASKS;
    max_task_duration= MAX_TASK_DURATION;
    time_unit= DEFAULT_TIME_STEP_DURATION;
    new_tasks= DEFAULT_NUM_NEW_TASKS;
    root= 0;
    report_rank= root;


    /* Check command line args */
    while ((ch= getopt(argc, argv, "di:m:r:s:t:v")) != EOF)   {
        switch (ch)   {
	    case 'd':
		debug++;
		break;

            case 'i':
		initial_tasks= strtol(optarg, (char **)NULL, 0);
		if (initial_tasks < 1)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Initial tasks (-i %d) needs to be >= 1\n", initial_tasks);
		    }
		    error= true;
		}
		break;

            case 'm':
		max_task_duration= strtol(optarg, (char **)NULL, 0);
		if (max_task_duration < 1)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Max task duration (-m %d) needs to be >= 1\n", max_task_duration);
		    }
		    error= true;
		}
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
		break;

            case 't':
		time_steps= strtol(optarg, (char **)NULL, 0);
		if (time_steps < 1)   {
		    if (my_rank == 0)   {
			fprintf(stderr, "Time steps (-t %d) needs to be >= 1\n", time_steps);
		    }
		    error= true;
		}
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
	printf("# Number of ranks is %d\n", num_ranks);
	printf("# Random number seed is %ld\n", seed);
	printf("# One time unit is %.9f s\n", units2seconds(1));
	printf("# Will do %d time units of work, ~%.6f s per trial\n",
	    time_steps, units2seconds(time_steps));
	printf("# Maximum task duration is %d time units, %.6f s\n",
	    max_task_duration, units2seconds(max_task_duration));
	printf("# Each rank gets an average of %d tasks to start\n", initial_tasks);
	printf("# When a tasks spawns more work, it will create an average of %d\n", new_tasks);
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
    stat_initial_work_time= 0;
    stat_new_tasks= 0;
    stat_new_task_requests_sent= 0;
    stat_new_task_requests_not_granted= 0;
    stat_new_task_requests_honored= 0;
    stat_new_task_requests_rcvd= 0;
    stat_new_task_requests_denied= 0;
    stat_new_task_requests_granted= 0;

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



static int
get_next_task(void)
{

int current_task= 0;


    if (!my_task_list->empty())   {
	current_task= my_task_list->front();
	my_task_list->pop_front();
    }
    return current_task;

}  // end of get_next_task()



static int
gen_new_task(void)
{

int duration;


    // Generate a task; i.e., determine a duration for it
    duration= (int) (drand48() * DEFAULT_TASK_DURATION);
    if (duration < 1)   {
	// No zero-length tasks, please
	duration= 1;
    }
    if (duration > max_task_duration)   {
	// We have an upper limit too
	duration= max_task_duration;
    }

    my_task_list->push_back(duration);

    return duration;

}  // end of gen_new_task() 



static void
assign_first_tasks(void)
{

int duration;


    // Give every rank an initial set of tasks
    num_start_tasks= (int)(-1.0 * log(drand48()) * initial_tasks);
    if (num_start_tasks < 1)   {
	// Everybody gets at least one task to start
	num_start_tasks= 1;
    }

    if (debug)   {
	printf("[%3d] Will start with %5d tasks\n", my_rank, num_start_tasks);
    }


    for (int i= 0; i < num_start_tasks; i++)   {
	duration= gen_new_task();
	stat_initial_work_time += duration;
	if (debug > 2)   {
	    printf("[%3d] Task %5d has duration %5d time units\n", my_rank, i, duration);
	}
    }

}  // end of assign_first_taks()



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
    // We did a time unit's worth of "work". Now do some
    // housekeeping

    // Lower the probability of new tasks being created
    // As time goes on, it should be less and less likely
    // that new tasks get created.
    p_task_creation= p_task_creation - 1.0 / time_steps;
    if (p_task_creation < 0.0)   {
	// This should not happen, but just in case
	p_task_creation= 0.0;
    }

    // Lower our back off threshold. We use it to determine whether it
    // is OK now to ask for more work.
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
	    stat_new_task_requests_rcvd++;

	    if (deny || my_task_list->size() < 5)   {
		// Sorry, no work for you
		MPI_Send(NULL, 0, MPI_BYTE, status.MPI_SOURCE, TAG_WORK_MIGRATION, MPI_COMM_WORLD);
		stat_new_task_requests_denied++;

	    } else   {
		// Pass off some work
		send_off_task= get_next_task();
		MPI_Isend(buf, send_off_task * MSG_SIZE_PER_WORK_UNIT, MPI_BYTE,
		    status.MPI_SOURCE, TAG_WORK_MIGRATION, MPI_COMM_WORLD, &req);
		stat_new_task_requests_granted++;
		pending_sends->push_back(req);
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
	    if (count > 0)   {
		// Add it to work list
		my_task_list->push_back(count / MSG_SIZE_PER_WORK_UNIT);
		stat_new_task_requests_honored++;

	    } else   {
		// If we get a decline, back off and increase the circle
		stat_new_task_requests_not_granted++;

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



// A task has completed. See if it spawned other
// tasks. Start the next task if one is in the queue.
bool static
finish_task(int *current_task)
{

int num_new_tasks;
bool busy;


    // We are done with this task
    stat_tasks_completed++;

    // See if the work of this task generated new work
    if (drand48() < p_task_creation)   {
	num_new_tasks= (int)(-1.0 * log(drand48()) * new_tasks);
	for (int i= 0; i < num_new_tasks; i++)   {
	    gen_new_task();
	    stat_new_tasks++;
	}
    }


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
	busy= true;
    }

    return busy;

}  // end of finish_task()



static void
request_more_work(char *buf, int max_work_size)
{

int num_new_tasks;
MPI_Request req;
int dest; int ask_tasks;


    // Send out some work requests to random nodes
    // Close ones initially, then farther away
    ask_tasks= DEFAULT_ASK_NEW_TASKS;
    num_new_tasks= (int)(-1.0 * log(drand48()) * ask_tasks);
    for (int i= 0; i < num_new_tasks; i++)   {

	dest= (int)(-1.0 * log(drand48()) * (circle % num_ranks));
	dest= dest - dest / 2;
	dest= (my_rank + dest + num_ranks) % num_ranks;
	assert(0 <= dest);
	assert(dest < num_ranks);

	if (dest == my_rank)   {
	    // No point in asking myself for more work
	    continue;
	}

	// Post a work receive for the new task or the decline
	MPI_Irecv(buf, max_work_size, MPI_BYTE, dest, TAG_WORK_MIGRATION, MPI_COMM_WORLD, &req);
	work_requests->push_back(req);

	// Send a work request
	MPI_Isend(NULL, 0, MPI_BYTE, dest, TAG_WORK_REQUEST, MPI_COMM_WORLD, &req);
	stat_new_task_requests_sent++;
	pending_sends->push_back(req);
    }

}  // end of request_more_work()



static void
clean_pending_comm(char *buf)
{

bool deny;
int cut_off;


    // At the end, we may have pending sends and receives that we
    // need to clean out of the system

    deny= true; // Don't send more work away
    cut_off= 0;
    while (!pending_sends->empty())   {
	clean_pending_sends();
	handle_work_requests(buf, deny);
	incoming_work();
	if (cut_off++ > 100000)   {
	    fprintf(stderr, "[%3d] WARNING!!! Not waiting any longer for sends to "
		"complete!\n", my_rank);
	    break;
	}
    }

    cut_off= 0;
    while (!work_requests->empty())   {
	handle_work_requests(buf, deny);
	incoming_work();
	if (cut_off++ > 100000)   {
	    fprintf(stderr, "[%3d] WARNING!!! Not waiting any longer for work requests "
		" to complete!\n", my_rank);
	    break;
	}
    }

    // FIXME:
    // I'm not sure how long to make the loop below. It should be long
    // to make sure there are no pending requests, but that delays completion
    // of the benchmark.
    // One idea is basically a non-blocking allreduce or barrier. After we have
    // all of our sends and requests are done, and our children have reported in,
    // send a done flag to my parent in the tree.  When I get it back that all
    // are done, I pass it on, and abort the loop below.

    // All our sends are out, and our work requests have been answered.
    // It is possible that more work requests are on the way to us
    // Hang around for a little while to answer them
    for (int i= 0; i < num_ranks * 50; i++)   {
	work(); // wait a while
	handle_work_requests(buf, deny);
    }

}  // end of clean_pending_comm()




double
ws_work(void)
{

int t;
int current_task;
bool busy;
char *work_buf;
int max_work_size;
bool deny;
double total_time_end;
double total_time_start;


    clear_stats();
    p_task_creation= 1.0;
    back_off= 0;
    circle= 1;
    srand48(seed * (my_rank + 1));

    max_work_size= MAX_TASK_DURATION * MSG_SIZE_PER_WORK_UNIT;
    work_buf= alloc_work_buf(max_work_size);

    my_task_list= new std::list <int>;
    pending_sends= new std::list <MPI_Request>;
    work_requests= new std::list <MPI_Request>;

    // Each rank gets a few tasks to start with
    assign_first_tasks();

    // Grab the first task on the list
    current_task= get_next_task();
    busy= true;


    // Sync and start timmer
    MPI_Barrier(MPI_COMM_WORLD);
    total_time_start= MPI_Wtime();

    for (t= 0; t < time_steps; t++)   {
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
	if (!busy && (back_off == 0))   {
	    request_more_work(work_buf, max_work_size);
	}
    }
    total_time_end= MPI_Wtime();

    if (busy)   {
	// I was working on a task at the end. Count it as completed
	stat_tasks_completed++;
    }
    clean_pending_comm(work_buf);

    free(work_buf);
    stat_tasks_left_in_queue= my_task_list->size();
    delete my_task_list;
    delete pending_sends;
    delete work_requests;

    MPI_Reduce(&stat_tasks_completed, &stat_total_tasks, 1, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);
    MPI_Reduce(&stat_time_worked, &stat_total_time_units, 1, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);
    elapsed= total_time_end - total_time_start;
    MPI_Reduce(&elapsed, &stat_elapsed, 1, MPI_DOUBLE, MPI_MAX, root, MPI_COMM_WORLD);

    // What do we want for a metric?
    // return elapsed;
    // return stat_elapsed;
    return stat_total_time_units;

}  /* end of ws_work() */



void
ws_print(void)
{

int cnt;


    // Some consistency checking
    if (stat_new_task_requests_sent !=
	stat_new_task_requests_not_granted + stat_new_task_requests_honored)   {
	fprintf(stderr, "[%3d] Comm pending in system: requests %d, granted %d, denied %d\n",
		my_rank, stat_new_task_requests_sent, stat_new_task_requests_honored,
		stat_new_task_requests_not_granted);
    }

    // Tasks available to me
    cnt= num_start_tasks + stat_new_tasks + stat_new_task_requests_honored;
    // Minus work done or given away
    cnt= cnt - stat_tasks_completed - stat_new_task_requests_granted;
    // Should equal what is left in queue
    if (cnt != stat_tasks_left_in_queue)   {
	fprintf(stderr, "[%3d] Work done and given away != work given + queue!\n", my_rank);
    }


    // Print some statistics
    if (my_rank == root)   {
	printf("#          Total time         %12.6f seconds\n", stat_elapsed);
	printf("#          Total tasks completed    %6d\n", stat_total_tasks);
	printf("#          Time units completed     %6d\n", stat_total_time_units);
    }


    if (my_rank == report_rank)   {
	printf("# Rank %3d My total time      %12.6f seconds\n", my_rank, elapsed);
	printf("# Rank %3d started with             %6d tasks,    %6d time units of work\n", my_rank, num_start_tasks, stat_initial_work_time);
	printf("# Rank %3d created                  %6d new tasks\n", my_rank, stat_new_tasks);
	printf("# Rank %3d completed                %6d tasks,    %6d left in queue\n", my_rank, stat_tasks_completed, stat_tasks_left_in_queue);
	printf("# Rank %3d worked                   %6d time units\n", my_rank, stat_time_worked);
	printf("# Rank %3d was idle                 %6d time units\n", my_rank, stat_time_idle);
	printf("# Rank %3d had                      %6d interrupts in nanosleep\n", my_rank, stat_interrupts);
	printf("# Rank %3d received                 %6d task requests from other ranks\n", my_rank, stat_new_task_requests_rcvd);
	printf("# Rank %3d denied                   %6d task requests to other ranks\n", my_rank, stat_new_task_requests_denied);
	printf("# Rank %3d granted                  %6d task requests to other ranks\n", my_rank, stat_new_task_requests_granted);
	printf("# Rank %3d sent                     %6d task requests to other ranks\n", my_rank, stat_new_task_requests_sent);
	printf("# Rank %3d had                      %6d task requests denied by other ranks\n", my_rank, stat_new_task_requests_not_granted);
	printf("# Rank %3d had                      %6d task requests honored by other ranks\n", my_rank, stat_new_task_requests_honored);
	printf("# Rank %3d back_off now             %6d, circle now %6d\n", my_rank, back_off, circle);
    }

}  /* end of ws_print() */



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-v] [-t time steps] [-m max] [-s seed] [-u unit] [-r rank]\n", pname);
    fprintf(stderr, "    -v               Increase verbosity\n");
    fprintf(stderr, "    -t time steps    Number of time steps to do. Default %d\n",
	DEFAULT_TIME_STEPS);
    fprintf(stderr, "    -m max           Max task duration in time steps. Default %d\n",
	MAX_TASK_DURATION);
    fprintf(stderr, "    -s seed          Random number seed to use. Default %d\n",
	DEFAULT_SEED);
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
