// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


// Author: Amro Awad (E-mail aawad@sandia.gov)
//
#include <sst_config.h>
#include "sst/core/elementinfo.h"
#include <Opal.h>

using namespace SST;
using namespace SST::OpalComponent;


static Component* create_Opal(ComponentId_t id, Params& params)
{
return new Opal(id,params);
};


const char *memEventList[] = {
  "MemEvent",
  NULL
};

static const ElementInfoParam Opal_params[] = {
    {"clock",              "Internal Controller Clock Rate.", "1.0 Ghz"},
    {"num_pools",	   "This determines the number of memory pools", "1"},
    {"corecount",	   "This determines the number of cores on each node", "1"},
    {"num_domains", "The number of domains in the system, typically similar to number of sockets/SoCs", "1"},
    {"allocation_policy",	   "0 is private pools, then clustered pools, then public pools", "0"},
    {"size%(num_pools)", "Size of each memory pool in KBs", "8388608"},
    {"startaddress%(num_pools)", "the starting physical address of the pool", "0"},
    {"type%(num_pools)", "0 means private for specific NUMA domain, 1 means shared among specific NUMA domains, 2 means public", "2"},
    {"cluster_size", "This determines the number of NUMA domains in each cluster, if clustering is used", "1"},
    {"memtype%(num_pools)", "0 for typical DRAM, 1 for die-stacked DRAM, 2 for NVM", "0"},
    {"typepriority%(num_pools)", "0 means die-stacked, typical DRAM, then NVM", "0"}, 
    { NULL, NULL }
};


static const ElementInfoStatistic Opal_statistics[] = {
    { "allocation_requests", "Determine the number of allocation requests", "requests", 1},
    { NULL, NULL, NULL, 0 }
};


static const ElementInfoPort Opal_ports[] = {
    {"request_commands%(corecount)d", "Each Samba link to its core", NULL},
    {NULL, NULL, NULL}
};


static const ElementInfoComponent components[] = {
    { "Opal",
      "Opal Component",
      NULL,
      create_Opal,
      Opal_params,
      Opal_ports,
      COMPONENT_CATEGORY_MEMORY,
      Opal_statistics
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo Opal_eli = {
        "Opal",
        "Opal Components",
        components,
    };
}
