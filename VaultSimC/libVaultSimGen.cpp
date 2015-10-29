// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include <VaultSimC.h>

extern "C" {
  Component* VaultSimCAllocComponent( SST::ComponentId_t id,  SST::Params& params );
  Component* create_logicLayer( SST::ComponentId_t id,  SST::Params& params );
}

const char *memEventList[] = {
  "MemEvent",
  NULL
};

static const ElementInfoParam VaultSimC_params[] = {
  {"clock",              "Vault Clock Rate.", "1.0 Ghz"},
  {"numVaults2",         "Number of bits to determine vault address (i.e. log_2(number of vaults per cube))"},
  {"VaultID",            "Vault Unique ID (Unique to cube)."},
  {"statistics",         "0: disable ,1: enable statistics printing"},
  {"debug",              "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
  {"debug_level",        "debug verbosity level (0-10)"},
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
  {"debug_level",        "debug verbosity level (0-10)"},
  { NULL, NULL }
};

static const ElementInfoPort logicLayer_ports[] = {
  {"bus_%(vaults)d", "Link to the individual memory vaults", memEventList},
  {"toCPU", "Connection towards the processor (directly to the proessor, or down the chain in the direction of the processor)", memEventList},    
  {"toMem", "If 'terminal' is 0 (i.e. this is not the last cube in the chain) then this port connects to the next cube.", memEventList},
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
  { NULL, NULL, NULL, NULL }
};

extern "C" {
  ElementLibraryInfo VaultSimC_eli = {
    "VaultSimC",
    "Stacked memory Vault Components",
    components,
  };
}
