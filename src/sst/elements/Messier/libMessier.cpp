// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"
#include <Messier.h>

using namespace SST;
using namespace SST::MessierComponent;


static Component* create_Messier(ComponentId_t id, Params& params)
{
return new Messier(id,params);
};


const char *memEventList[] = {
  "MemEvent",
  NULL
};

static const ElementInfoParam Messier_params[] = {
    {"clock",              "Internal Controller Clock Rate.", "1.0 Ghz"},
    {"memory_clock",              "NVM Chips Clock Rate.", "1.0 Ghz"},
    {"size",         "The size of the whole NVM-Based DIMM in KBs", "8388608"},
    {"write_buffer_size",            "The size of the internal write buffer (# entries) ", "64"},
    {"max_requests",              "This determines the max number of requests can be buffered at the NVMDIMM", "64"},
    {"max_outstanding", "This determines the max number of outstanding/executing requests inside the NVMDIMM", "16"},
    {"max_current_weight", "Determines the power limit, which in turn limits the outstanding requests", "120"},
    {"write_weight", "Determines the average power weight of a currently executing write operation", "15"},
    {"read_weight", "Determines the average power weight of a currently executing read operation", "3"},
    {"tCMD", "The time needed to send a command from the controller to the NVM-Chip (in controller cycles)", "1"},
    {"tCL", "The time needed to read a word from an already activated row (in controller cycles)", "70" },    
    {"tCL_W", "The time needed to write a word completely to the PCM Chip (in controller cycles)", "1000"},
    {"tRCD", "The time needed to activate a row in PCM chips (in controller cycles)", "300"},
    {"tBURST", "The time needed for submitting a data burst on the bus", "7"},
    {"device_width", "This determines the NVM-Chip output width", "8"},
    {"num_devices", "The number of NVM-Chip devices", "8"},
    {"num_ranks", "The number of ranks", "1"},
    {"num_banks", "The number of banks on each rank", "16"},
    {"row_buffer_size", "The row buffer size in bytes", "8192"},
    {"flush_th", "This determines the percentage of max number of entries in write buffer that triggers flushing it", "50"},
    {"flush_th_low", "This determines the percentage of the low threshold number of entries", "30"},
    {"max_writes", "This determines the maximum number of concurrent writes at NVM chips (not including those on buffer", "1"}, 
    {"cacheline_interleaving", "This determines if cacheline interleaving is used (1) or bank interleaving (0) ", "1"}, 
    {"adaptive_writes", "This indicates that the writes rate is adjusted dynamically", "0"},
    { NULL, NULL }
};

static const ElementInfoStatistic Messier_statistics[] = {
    { "reads", "Determine the number of reads", "reads", 1},
    { "writes", "Determine the number of writes", "writes", 1},
    { "avg_time", "The average time spent on each read request", "cycles", 3},
    { "histogram_idle", "The histogram of cycles length while controller is idle", "cycles",1},
    { NULL, NULL, NULL, 0 }
};


static const ElementInfoPort Messier_ports[] = {
    {"bus", "Link to the socket controller", NULL},
    {NULL, NULL, NULL}
};


static const ElementInfoComponent components[] = {
    { "Messier",
      "Messier Component",
      NULL,
      create_Messier,
      Messier_params,
      Messier_ports,
      COMPONENT_CATEGORY_MEMORY,
      Messier_statistics
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo Messier_eli = {
        "Messier",
        "Messier Components",
        components,
    };
}
