#include <sst_config.h>
#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include <VaultSimC.h>

extern "C" {
  Component* VaultSimCAllocComponent( SST::ComponentId_t id,  SST::Params& params );
  Component* create_logicLayer( SST::ComponentId_t id,  SST::Params& params );
  Component* create_cpu( SST::ComponentId_t id,  SST::Params& params );
}


static const ElementInfoParam VaultSimC_params[] = {
  {"clock",              "Vault Clock Rate."},
  {"numVaults2",         "Number of bits to determine vault address (i.e. log_2(number of vaults per cube))"},
  {"VaultID",            "Vault Unique ID (Unique to cube)."},
  {"debug",              "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."}
};

static const ElementInfoParam logicLayer_params[] = {
  {"clock",              "Logic Layer Clock Rate."},
  {"llID",               "Logic Layer ID (Unique id within chain)"},
  {"bwlimit",            "Number of memory events which can be processed per cycle per link."},
  {"LL_MASK",            "Bitmask to determine 'ownership' of an address by a cube. A cube 'owns' an address if ((((addr >> LL_SHIFT) & LL_MASK) == llID) || (LL_MASK == 0)). LL_SHIFT is set in vaultGlobals.h and is 8 by default."},
  {"terminal",           "Is this the last cube in the chain?"},
  {"vaults",             "Number of vaults per cube."},
  {"debug",              "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."}
};

static const ElementInfoParam cpu_params[] = {
  {"clock",              "Simple CPU Clock Rate."},
  {"threads",            "Number of simulated threads in cpu."},
  {"app",                "Synthetic Application. 0:miniMD-like 1:phdMesh-like. (See app.cpp for details)."},
  {"bwlimit",            "Maximum number of memory instructions issued by the processor per cycle. Note, each thread can only have at most 2 outstanding memory references at a time. "},
  {"seed",               "Optional random number generator seed. If not defined or 0, uses srandomdev()."}
};

static const ElementInfoComponent components[] = {
    { "VaultSimC",
      "Vault Component",
      NULL,
      VaultSimCAllocComponent,
      VaultSimC_params
    },
    { "logicLayer",
      "Logic Layer Component",
      NULL,
      create_logicLayer,
      logicLayer_params
    },
    { "cpu",
      "simple CPU",
      NULL,
      create_cpu,
      cpu_params
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
