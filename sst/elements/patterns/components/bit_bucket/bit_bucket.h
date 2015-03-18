// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// This is a component that consumes events delivered by a routermodel
// We use it to model a storage device. It will acknolwedge incoming
// data after some write dealy. It will deliver read data after
// receiving a request and the appropriate delay.


#ifndef _BIT_BUCKET_H
#define _BIT_BUCKET_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS	(1)
#endif
#include <inttypes.h>		// For PRIu64


#include <sst/core/component.h>
#include <sst/core/element.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include "sst/core/serialization.h"



using namespace SST;

#define DBG_BIT_BUCKET 1
#if DBG_BIT_BUCKET
#define _BIT_BUCKET_DBG(lvl, fmt, args...)\
    if (bit_bucket_debug >= lvl)   { \
	printf("%d:Bit_bucket::%s():%d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _BIT_BUCKET_DBG(lvl, fmt, args...)
#endif



// Set this to start at 200 and use this fixed order
// That way we can "merge" enums with the pattern generator event enums
typedef enum {BIT_BUCKET_WRITE_START= 200, BIT_BUCKET_WRITE_DONE, BIT_BUCKET_READ_START,
    BIT_BUCKET_READ_DONE} bit_bucket_op_t;


class Bit_bucket : public Component {
    public:
        Bit_bucket(ComponentId_t id, Params& params) :
	    Component(id),
            params(params)
        {
            Params::iterator it= params.begin();
	    bit_bucket_debug= 0;
	    read_pipe= 0;
	    write_pipe= 0;
	    write_bw= 0;
	    read_bw= 0;
	    bytes_written= 0;
	    bytes_read= 0;
	    total_read_delay= 0;
	    total_write_delay= 0;
	    total_reads= 0;
	    total_writes= 0;


            while (it != params.end())   {
		if (!SST::Params::getParamName(it->first).compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &bit_bucket_debug);
		
		} else if (!SST::Params::getParamName(it->first).compare("write_bw"))   {
		    sscanf(it->second.c_str(), "%" PRIu64, (uint64_t *)&write_bw);
		
		} else if (!SST::Params::getParamName(it->first).compare("read_bw"))   {
		    sscanf(it->second.c_str(), "%" PRIu64, (uint64_t *)&read_bw);
		}
						                                                                                                        
                ++it;
            }

	    // Check the parameters
	    if ((write_bw == 0) || (read_bw == 0))   {
		_BIT_BUCKET_DBG(0, "You need to supply a write_bw and read_bw in the configuration!\n");
		_ABORT(Bit_bucket, "Check the input XML file!\n");
	    }


	    // Create a time converter for this bit bucket
	    TimeConverter *tc= registerTimeBase("1ns", true);

	    // Create our link to the network
	    net= configureLink("STORAGE", new Event::Handler<Bit_bucket>
		    (this, &Bit_bucket::handle_events));
	    if (net == NULL)   {
		_BIT_BUCKET_DBG(0, "Expected a link named \"STORAGE\" which is missing!\n");
		_ABORT(Bit_bucket, "Check the input XML file!\n");
	    }

            // Create a channel for "out of band" events sent to ourselves
	    self_link= configureSelfLink("Me", new Event::Handler<Bit_bucket>
		    (this, &Bit_bucket::handle_events));
	    if (self_link == NULL)   {
		_ABORT(Ghost_pattern, "That was no good!\n");
	    }

	    net->setDefaultTimeBase(tc);
	    self_link->setDefaultTimeBase(tc);
        }



    private:

        Bit_bucket() {}; // For serialization only
        Bit_bucket(const Bit_bucket &c);
	void handle_events(Event *);

        Params params;
	Link *net;
	Link *self_link;
	int bit_bucket_debug;
	SimTime_t read_pipe;
	SimTime_t write_pipe;
	uint64_t write_bw;
	uint64_t read_bw;
	uint64_t bytes_written;
	uint64_t bytes_read;
	uint64_t total_read_delay;
	uint64_t total_write_delay;
	uint64_t total_reads;
	uint64_t total_writes;


        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(net);
	    ar & BOOST_SERIALIZATION_NVP(self_link);
	    ar & BOOST_SERIALIZATION_NVP(bit_bucket_debug);
	    ar & BOOST_SERIALIZATION_NVP(read_pipe);
	    ar & BOOST_SERIALIZATION_NVP(write_pipe);
	    ar & BOOST_SERIALIZATION_NVP(write_bw);
	    ar & BOOST_SERIALIZATION_NVP(read_bw);
	    ar & BOOST_SERIALIZATION_NVP(bytes_written);
	    ar & BOOST_SERIALIZATION_NVP(bytes_read);
	    ar & BOOST_SERIALIZATION_NVP(total_read_delay);
	    ar & BOOST_SERIALIZATION_NVP(total_write_delay);
	    ar & BOOST_SERIALIZATION_NVP(total_reads);
	    ar & BOOST_SERIALIZATION_NVP(total_writes);
        }
};

#endif  // _BIT_BUCKET_H_
