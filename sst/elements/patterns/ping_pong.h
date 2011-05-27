// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _PINGPONG_PATTERN_H
#define _PINGPONG_PATTERN_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include "gate_keeper.h"
#include "pattern_common.h"

using namespace SST;

#define DBG_PINGPONG_PATTERN 1
#if DBG_PINGPONG_PATTERN
#define _PINGPONG_PATTERN_DBG(lvl, fmt, args...)\
    if (pingpong_pattern_debug >= lvl)   { \
	printf("%d:Pingpong_pattern::%s():%4d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _PINGPONG_PATTERN_DBG(lvl, fmt, args...)
#endif

typedef enum {INIT,			// First state in state machine
	      DONE,			// Work is all done
} state_t;



class Pingpong_pattern : public Component {
    public:
        Pingpong_pattern(ComponentId_t id, Params_t& params) :
	    Component(id),
	    params(params)
	{

	    // Create a gate keeper and initalize it
	    gate= new Gate_keeper();
	    gate->init(this, id, params);

	    // Defaults for pattern-specific paramters
	    pingpong_pattern_debug= 0;
	    num_msgs= 20;
	    exchange_msg_len= 0;

	    // Process pattern specific parameters
	    Params_t::iterator it= params.begin();
            while (it != params.end())   {
                _PINGPONG_PATTERN_DBG(2, "[%3d] key \"%s\", value \"%s\"\n", gate->my_rank,
		    it->first.c_str(), it->second.c_str());

		if (!it->first.compare("num_msgs"))   {
		    sscanf(it->second.c_str(), "%d", &num_msgs);
		}

		if (!it->first.compare("exchange_msg_len"))   {
		    sscanf(it->second.c_str(), "%d", &exchange_msg_len);
		}

                ++it;
            }

	    gate->start();

        }

	void handle_events(pattern_event_t event);

    private:
        Pingpong_pattern(const Pingpong_pattern &c);
	Params_t params;
	Gate_keeper *gate;

	int pingpong_pattern_debug;
	int num_msgs;
	int exchange_msg_len;




        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
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
