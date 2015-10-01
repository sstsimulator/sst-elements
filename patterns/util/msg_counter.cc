//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <string.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS	(1)
#include <inttypes.h>		// For PRIu64
#include "msg_counter.h"



void
Msg_counter::record(int rank, uint64_t len)
{

counter_t i;
std::set<counter_t>::iterator p;


    if ((rank < 0) || (rank >= max_rank))   {
	fprintf(stderr, "Error: rank %d must be 0...%d\n", rank, max_rank);
	return;
    }

    i.rank= rank;
    p= counters.find(i);
    if (p == counters.end())   {
	// Not in the list yet, insert it
	i.messages= 1;
	i.bytes= len;
	counters.insert(i);
    } else   {
	// Update with new data
	// FIXME: Is this the best (only) way to do this?
	// See also record_send in NIC_stats.cc with the same problem
	i.messages= p->messages + 1;
	i.bytes= p->bytes + len;
	counters.erase(p);
	counters.insert(i);
    }

}  // end of record()



void
Msg_counter::insert_log_entry(int rank, uint64_t len, int time_step)
{

log_entry_t entry;


    if ((rank < 0) || (rank >= max_rank))   {
	fprintf(stderr, "Error: rank %d must be 0...%d\n", rank, max_rank);
	return;
    }

    entry.rank= rank;
    entry.time_step= time_step;
    entry.bytes= len;
    log.push_back(entry);

}  // end of insert_log_entry()



void
Msg_counter::clear(void)
{
    counters.clear();
}  // end of clear()



uint64_t
Msg_counter::total_cnt(void)
{

std::set<counter_t>::iterator it;
uint64_t total;


    total= 0;
    for (it= counters.begin(); it != counters.end(); it++)   {
	total= total + it->messages;
    }

    return total;

}  // end of total_cnt()



void
Msg_counter::output_cnt(const char *fname)
{
    output(fname, true);
}  // end of output_cnt()



void
Msg_counter::output_bytes(const char *fname)
{
    output(fname, false);
}  // end of output_bytes()



void
Msg_counter::output(const char *fname, bool cnt)
{

std::set<counter_t>::iterator it;
int expect_rank;
int field_width;
FILE *fp;


    fp= fopen(fname, "a");
    if (fp == NULL)   {
	fprintf(stderr, "Could not open \"%s\" for writing message count data!\n", fname);
	fprintf(stderr, "    %s\n", strerror(errno));
	return;
    }


    field_width= 4;
    expect_rank= 0;
    for (it= counters.begin(); it != counters.end(); it++)   {
	if ((it->rank < 0) || (it->rank >= max_rank))   {
	    fprintf(stderr, "\nError: rank %d\n", it->rank);
	    return;
	}

	while (expect_rank < it->rank)   {
	    // No entry for that rank in the set, print a 0
	    fprintf(fp, "%*d ", field_width, 0);
	    expect_rank++;
	}
	if (cnt)   {
	    fprintf(fp, "%*" PRIu64 " ", field_width, it->messages);
	} else   {
	    fprintf(fp, "%*" PRIu64 " ", field_width, it->bytes);
	}
	expect_rank++;
    }

    while (expect_rank < max_rank)   {
	// there might be trailing entries we need to fill
	fprintf(fp, "%*d ", field_width, 0);
	expect_rank++;
    }
    fprintf(fp, "\n");
    fclose(fp);

    if (expect_rank > max_rank)   {
	fprintf(stderr, "\nError in %s: rank > max_rank %d\n", __FUNCTION__, max_rank);
    }

}  // end of output()



void
Msg_counter::show_log(const char *fname)
{

std::list<log_entry_t>::iterator it;
FILE *fp;
int cnt;


    fp= fopen(fname, "a");
    if (fp == NULL)   {
	fprintf(stderr, "Could not open \"%s\" for writing message count data!\n", fname);
	fprintf(stderr, "    %s\n", strerror(errno));
	return;
    }


    cnt= 1;
    for (it= log.begin(); it != log.end(); it++)   {
	if ((it->rank < 0) || (it->rank >= max_rank))   {
	    fprintf(stderr, "\nError: rank 0 <= %d < %d\n", it->rank, max_rank);
	    return;
	}

	if (it->bytes > 0)   {
	    // Only show actual task migrations
	    fprintf(fp, "%7d %5d %3d %" PRIu64 "\n", cnt, it->time_step, it->rank, it->bytes);
	}
	cnt++;
    }

    fclose(fp);

}  // end of show_log()

#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(Msg_counter)
#endif // SERIALIZATION_WORKS_NOW
