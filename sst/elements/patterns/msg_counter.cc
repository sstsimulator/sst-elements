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



// Am I the root of the tree?
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
	// See also event_send in pattern_common.cc with the same problem
	i.messages= p->messages + 1;
	i.bytes= p->bytes + len;
	counters.erase(p);
	counters.insert(i);
    }

}  // end of record()



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
Msg_counter::output_cnt(int num_ranks, const char *fname)
{
    output(num_ranks, fname, true);
}  // end of output_cnt()



void
Msg_counter::output_bytes(int num_ranks, const char *fname)
{
    output(num_ranks, fname, false);
}  // end of output_bytes()



void
Msg_counter::output(int num_ranks, const char *fname, bool cnt)
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
	if ((it->rank < 0) || (it->rank >= num_ranks))   {
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

    while (expect_rank < num_ranks)   {
	// there might be trailing entries we need to fill
	fprintf(fp, "%*d ", field_width, 0);
	expect_rank++;
    }
    fprintf(fp, "\n");
    fclose(fp);

    if (expect_rank > num_ranks)   {
	fprintf(stderr, "\nError in %s: rank > num_ranks %d\n", __FUNCTION__, num_ranks);
    }

}  // end of output()
