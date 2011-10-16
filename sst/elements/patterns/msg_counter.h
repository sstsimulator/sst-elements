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
#include <sst_config.h>
#include <boost/serialization/set.hpp>
#include <boost/serialization/list.hpp>
#include <sst/core/serialization/element.h>


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

	void output(const char *fname, bool cnt);

	typedef struct   {
	    int rank;
	    uint64_t messages;
	    uint64_t bytes;

	    friend class boost::serialization::access;
	    template<class Archive>
	    void serialize(Archive & ar, const unsigned int version)
	    {
		ar & BOOST_SERIALIZATION_NVP(rank);
		ar & BOOST_SERIALIZATION_NVP(messages);
		ar & BOOST_SERIALIZATION_NVP(bytes);
	    }
	} counter_t;

	typedef struct   {
	    int rank;
	    int time_step;
	    uint64_t bytes;

	    friend class boost::serialization::access;
	    template<class Archive>
	    void serialize(Archive & ar, const unsigned int version)
	    {
		ar & BOOST_SERIALIZATION_NVP(rank);
		ar & BOOST_SERIALIZATION_NVP(time_step);
		ar & BOOST_SERIALIZATION_NVP(bytes);
	    }
	} log_entry_t;

	struct _compare   {
	    bool operator () (const counter_t& x, const counter_t& y) const {return x.rank < y.rank;}
	};

	std::set<counter_t, _compare> counters;
	std::list<log_entry_t> log;
	int max_rank;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
	    ar & BOOST_SERIALIZATION_NVP(counters);
	    ar & BOOST_SERIALIZATION_NVP(log);
	    ar & BOOST_SERIALIZATION_NVP(max_rank);
        }

};

#endif // _MESSAGE_COUNTER_H_
