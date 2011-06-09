//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _PINGPONG_PATTERN_H
#define _PINGPONG_PATTERN_H

#include "comm_pattern.h"


class Pingpong_pattern : public Comm_pattern {
    public:
        Pingpong_pattern(ComponentId_t id, Params_t& params) :
	    // constructor initializer list                                                                   
	    Comm_pattern(id, params)
	{
	    // Place the target as far away as possible in a torus
	    dest= NetWidth() * NetHeight() * NoCWidth() * NoCHeight() * NumCores() / 2;

	    // Process the ping/pong pattern specific parameters
	    Params_t::iterator it= params.begin();

	    while (it != params.end())   {
		if (!it->first.compare("destination"))   {
		    sscanf(it->second.c_str(), "%d", &dest);
		}
		it++;
	    }

	    register_app_pattern(new Event::Handler<Pingpong_pattern>
		(this, &Pingpong_pattern::handle_events));
        }

        ~Pingpong_pattern()
	{
	}

    private:
	Pingpong_pattern(const Pingpong_pattern &c);
	void handle_events(Event *sst_event);
	Params_t params;

	int cnt;
	int done;
	int len;
	SimTime_t start_time;
	int first_receive;
	int dest;

        template<class Archive>
        friend void save_construct_data(Archive & ar,
                                        const Pingpong_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Pingpong_pattern,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void load_construct_data(Archive & ar,
                                        Pingpong_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Pingpong_pattern,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Pingpong_pattern(id, params);
        }

};

#endif // _PINGPONG_PATTERN_H
