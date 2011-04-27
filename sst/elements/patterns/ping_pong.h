// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _PINGPONG_PATTERN_H
#define _PINGPONG_PATTERN_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
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

	    // Defaults for paramters
	    pingpong_pattern_debug= 0;

        }

	void handle_events(pattern_event_t event);

    private:
	Params_t params;

        Pingpong_pattern(const Pingpong_pattern &c);

	int pingpong_pattern_debug;




        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(pingpong_pattern_debug);
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
