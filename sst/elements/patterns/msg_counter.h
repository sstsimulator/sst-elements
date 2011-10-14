//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _MESSAGE_COUNTER_H_
#define _MESSAGE_COUNTER_H_

#include <stdio.h>
#include <stdint.h>	// For uint64_t
#include <set>
#include <list>


class Msg_counter   {
    public:
	Msg_counter(int max) : max_rank(max) {} 
        ~Msg_counter() {}

	void clear(void);
	void record(int rank, uint64_t len);
	void insert_log_entry(int rank, uint64_t len, int time_step);
	void output_cnt(const char *fname);
	void output_bytes(const char *fname);
	void show_log(const char *fname);
	uint64_t total_cnt(void);

    private:
	typedef struct counter_t   {
	    int rank;
	    uint64_t messages;
	    uint64_t bytes;
	} counter_t;

	typedef struct log_entry_t   {
	    int rank;
	    int time_step;
	    uint64_t bytes;
	} log_entry_t;

	struct _compare   {
	    bool operator () (const counter_t& x, const counter_t& y) const {return x.rank < y.rank;}
	};

	std::set<counter_t, _compare> counters;
	std::list<log_entry_t> log;
	void output(const char *fname, bool cnt);
	int max_rank;
};

#endif // _MESSAGE_COUNTER_H_
