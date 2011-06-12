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
	    // Messages are exchanged between rank 0 and "dest"
	    // The default for "dest" is to place it as far away
	    // as possible in the (logical) torus created by the
	    // Comm_pattern object
	    dest= NetWidth() * NetHeight() * NoCWidth() * NoCHeight() * NumCores() / 2;

	    // Set some more defaults
	    num_msg= 10;
	    end_len= 1024;
	    len_inc= 8;



	    // Process the ping/pong pattern specific parameters
	    Params_t::iterator it= params.begin();

	    while (it != params.end())   {
		if (!it->first.compare("destination"))   {
		    sscanf(it->second.c_str(), "%d", &dest);
		}

		if (!it->first.compare("num_msg"))   {
		    sscanf(it->second.c_str(), "%d", &num_msg);
		}

		if (!it->first.compare("end_len"))   {
		    sscanf(it->second.c_str(), "%d", &end_len);
		}

		if (!it->first.compare("len_inc"))   {
		    sscanf(it->second.c_str(), "%d", &len_inc);
		}

		it++;
	    }

	    // Let Comm_pattern know which handler we want to have called
	    register_app_pattern(new Comm_pattern::PatternHandler<Pingpong_pattern>
		(this, &Pingpong_pattern::handle_events));
        }

        ~Pingpong_pattern()
	{
	}

    private:
	Pingpong_pattern(const Pingpong_pattern &c);
	void handle_events(int sst_event);
	Params_t params;

	int cnt;
	int done;
	int len;
	int end_len;
	int num_msg;
	int len_inc;
	SimTime_t start_time;
	int first_receive;
	int dest;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(cnt);
	    ar & BOOST_SERIALIZATION_NVP(done);
	    ar & BOOST_SERIALIZATION_NVP(len);
	    ar & BOOST_SERIALIZATION_NVP(end_len);
	    ar & BOOST_SERIALIZATION_NVP(num_msg);
	    ar & BOOST_SERIALIZATION_NVP(len_inc);
	    ar & BOOST_SERIALIZATION_NVP(start_time);
	    ar & BOOST_SERIALIZATION_NVP(first_receive);
	    ar & BOOST_SERIALIZATION_NVP(dest);
	}

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
