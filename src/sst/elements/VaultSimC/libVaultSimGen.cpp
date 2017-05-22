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
#include <VaultSimC.h>

using namespace SST;
using namespace SST::VaultSim;

extern "C" {
  Component* VaultSimCAllocComponent( SST::ComponentId_t id,  SST::Params& params );
  Component* create_logicLayer( SST::ComponentId_t id,  SST::Params& params );
  Component* create_cpu( SST::ComponentId_t id,  SST::Params& params );
}

const char *memEventList[] = {
  "MemEvent",
  NULL
};

static const ElementInfoParam VaultSimC_params[] = {
    {"clock",              "Vault Clock Rate.", "1.0 Ghz"},
    {"numVaults2",         "Number of bits to determine vault address (i.e. log_2(number of vaults per cube))"},
    {"VaultID",            "Vault Unique ID (Unique to cube)."},
    {"debug",              "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
#if !(HAVE_LIBPHX == 1)
    {"delay", "Static vault delay", "40ns"},
#endif /* HAVE_LIBPHX */
  { NULL, NULL }
};

static const ElementInfoPort VaultSimC_ports[] = {
    {"bus", "Link to the logic layer", memEventList},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic VaultSimC_statistics[] = {
    { "Mem_Outstanding", "Number of memory requests outstanding each cycle", "reqs/cycle", 1},
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoStatistic logicLayer_statistics[] = {
    { "BW_recv_from_CPU", "Bandwidth used (recieves from the CPU by the LL) per cycle (in messages)", "reqs/cycle", 1},
    { "BW_send_to_CPU", "Bandwidth used (sends from the CPU by the LL) per cycle (in messages)", "reqs/cycle", 2},
    { "BW_recv_from_Mem", "Bandwidth used (recieves from other memories by the LL) per cycle (in messages)", "reqs/cycle", 3},
    { "BW_send_to_Mem", "Bandwidth used (sends from other memories by the LL) per cycle (in messages)", "reqs/cycle", 4},
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoParam logicLayer_params[] = {
  {"clock",              "Logic Layer Clock Rate."},
  {"llID",               "Logic Layer ID (Unique id within chain)"},
  {"bwlimit",            "Number of memory events which can be processed per cycle per link."},
  {"LL_MASK",            "Bitmask to determine 'ownership' of an address by a cube. A cube 'owns' an address if ((((addr >> LL_SHIFT) & LL_MASK) == llID) || (LL_MASK == 0)). LL_SHIFT is set in vaultGlobals.h and is 8 by default."},
  {"terminal",           "Is this the last cube in the chain?"},
  {"vaults",             "Number of vaults per cube."},
  {"debug",              "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
  { NULL, NULL }
};

static const ElementInfoPort logicLayer_ports[] = {
    {"bus_%(vaults)d", "Link to the individual memory vaults", memEventList},
    {"toCPU", "Connection towards the processor (directly to the proessor, or down the chain in the direction of the processor)", memEventList},    
    {"toMem", "If 'terminal' is 0 (i.e. this is not the last cube in the chain) then this port connects to the next cube.", memEventList},
    {NULL, NULL, NULL}
};

static const ElementInfoParam cpu_params[] = {
  {"clock",              "Simple CPU Clock Rate."},
  {"threads",            "Number of simulated threads in cpu."},
  {"app",                "Synthetic Application. 0:miniMD-like 1:phdMesh-like. (See app.cpp for details)."},
  {"bwlimit",            "Maximum number of memory instructions issued by the processor per cycle. Note, each thread can only have at most 2 outstanding memory references at a time. "},
  {"seed",               "Optional random number generator seed. If not defined or 0, uses srandomdev()."},
  { NULL, NULL }
};

static const ElementInfoPort cpu_ports[] = {
    {"toMem", "Link to the memory system", memEventList},
    {NULL, NULL, NULL}
};

static const ElementInfoComponent components[] = {
    { "VaultSimC",
      "Vault Component",
      NULL,
      VaultSimCAllocComponent,
      VaultSimC_params,
      VaultSimC_ports,
      COMPONENT_CATEGORY_MEMORY,
      VaultSimC_statistics
    },
    { "logicLayer",
      "Logic Layer Component",
      NULL,
      create_logicLayer,
      logicLayer_params,
      logicLayer_ports,
      COMPONENT_CATEGORY_MEMORY,
      logicLayer_statistics
    },
    { "cpu",
      "simple CPU",
      NULL,
      create_cpu,
      cpu_params,
      cpu_ports,
      COMPONENT_CATEGORY_PROCESSOR
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo VaultSimC_eli = {
        "VaultSimC",
        "Stacked memory Vault Components",
        components,
    };
}
