
#include <sst_config.h>

#include "sst/core/element.h"
#include "sst/core/component.h"

#include "savcomp.h"
#include "savarb.h"
#include "arbitrator/savfifoarb.h"

using namespace SST;
using namespace SST::Savannah;

static Component* create_SavannahComponent(ComponentId_t id, Params& params) {
	return new SavannahComponent(id, params);
}

static SubComponent* create_InOrderArbitrator(Component* comp, Params& params) {
	return new SavannahInOrderArbitrator(comp, params);
}

static const ElementInfoParam savannah_params[] = {
    { NULL, NULL, NULL }
};

static const ElementInfoSubComponent subcomponents[] = {
	{
		"InOrderArbitrator",
		"First-in, First-out request issue",
		NULL,
		create_InOrderArbitrator,
		NULL, // params
		NULL, // Statistics
		"SST::Savannah::SavannahRequestArbitrator"
	},
    	{ NULL, NULL, NULL, NULL, NULL, NULL }
};

static const ElementInfoComponent components[] = {
	{
		"SavannahComponent",
        	"Multi-point Memory Controller and Request Arbitration",
		NULL,
		create_SavannahComponent,
		savannah_params,
//        	prospero_ports,
		NULL,
        	COMPONENT_CATEGORY_MEMORY
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

extern "C" {
	ElementLibraryInfo savannah_eli = {
		"savannah",
		"Multi-point Memory Connectors",
		components,
        	NULL, // Events
        	NULL, // Introspectors
        	NULL, // Modules
		subcomponents,
		NULL,
		NULL
	};
}

