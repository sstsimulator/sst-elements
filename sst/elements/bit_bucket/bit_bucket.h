// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2010, Sandia Corporation
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

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>


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



class Bit_bucket : public Component {
    public:
        Bit_bucket(ComponentId_t id, Params_t& params) :
	    Component(id),
            params(params)
        {
            Params_t::iterator it= params.begin();
	    bit_bucket_debug= 0;


            while (it != params.end())   {
		if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &bit_bucket_debug);
		}

                ++it;
            }


	    // Create a time converter for this bit bucket
	    TimeConverter *tc= registerTimeBase("1ns", true);


	    net= configureLink("STORAGE", new Event::Handler<Bit_bucket>
		    (this, &Bit_bucket::handle_events));
	    if (net == NULL)   {
		_BIT_BUCKET_DBG(0, "Expected a link named \"STORAGE\" which is missing!\n");
		_ABORT(Bit_bucket, "Check the input XML file!\n");
	    }

	    net->setDefaultTimeBase(tc);
        }



    private:

        Bit_bucket(const Bit_bucket &c);
	void handle_events(Event *);

        Params_t params;
	Link *net;
	int bit_bucket_debug;


        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar,
                                        const Bit_bucket * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Bit_bucket,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void load_construct_data(Archive & ar,
                                        Bit_bucket * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Bit_bucket,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Bit_bucket(id, params);
        }
};

#endif  // _BIT_BUCKET_H_
