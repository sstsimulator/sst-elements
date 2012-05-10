#include "core/zesto-core.h"
#include "core/trace-core.h"
#ifdef USE_QSIM
#include "core/qsim/qsimlib-core.h"
#include "core/qsim/qsimclient-core.h"
#endif
#include "simpleCache/simple_cache.h"
#include "simpleMC/simple_mc.h"
#include "irisNIC/irisNic.h"

BOOST_CLASS_EXPORT(simple_cache)
BOOST_CLASS_EXPORT(cache_req)
BOOST_CLASS_EXPORT(mem_req)
BOOST_CLASS_EXPORT(irisNPkt)
BOOST_CLASS_EXPORT(irisRtrEvent)

static SST::Component*
create_simple_cache(SST::ComponentId_t id, SST::Component::Params_t & params)
{
	return new simple_cache(id, params);
}

static SST::Component*
create_simple_mc(SST::ComponentId_t id, SST::Component::Params_t & params)
{
	return new SimpleMC(id, params);
}

static SST::Component*
create_trace_core_t(SST::ComponentId_t id, SST::Component::Params_t & params)
{
	return new trace_core_t(id, params);
}

#ifdef USE_QSIM
static SST::Component*
create_qsimlib_core_t(SST::ComponentId_t id, SST::Component::Params_t & params)
{
	return new qsimlib_core_t(id, params);
}

static SST::Component*
create_qsimclient_core_t(SST::ComponentId_t id, SST::Component::Params_t & params)
{
	return new qsimclient_core_t(id, params);
}
#endif

static SST::Component*
create_iris_nic(SST::ComponentId_t id, SST::Component::Params_t & params)
{
    if(params.find("event_type") != params.end()) {
        if(params["event_type"].compare("mem_req")==0)
            return new irisNic<mem_req>(id, params);
        else
            _abort(iris_nic, "unsupported terminal type, currently only support \"mem_req\"!\n");
    }
    else
        _abort(iris_nic, "terminal type not found, specify it by \"event_type\"!\n");
}

static const SST::ElementInfoComponent components[] = {
	{ "trace_core_t",
	  "Zesto core with trace frontend",
	  NULL,
	  create_trace_core_t
	},
#ifdef USE_QSIM
	{ "qsimlib_core_t",
	  "Zesto core with qsim lib frontend",
	  NULL,
	  create_qsimlib_core_t
	},
	{ "qsimclient_core_t",
	  "Zesto core with qsim client frontend",
	  NULL,
	  create_qsimclient_core_t
	},
#endif
	{ "simpleCache",
	  "A simple cache module for zesto",
	  NULL,
	  create_simple_cache
	},
	{ "simpleMC",
	  "A simple memory controller module for zesto",
	  NULL,
	  create_simple_mc
	},
    { "irisNic",
      "Network interface controller for iris",
      NULL,
      create_iris_nic
    },

	{NULL, NULL, NULL, NULL}
};

extern "C" {
	SST::ElementLibraryInfo zesto_eli = {
		"zesto",
		"Zesto simulator",
		components,
	};
}
