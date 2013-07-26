#include <sst_config.h>
#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include <VaultSimC.h>

extern "C" {
  Component* VaultSimCAllocComponent( SST::ComponentId_t id,  SST::Component::Params_t& params );
  Component* create_logicLayer( SST::ComponentId_t id,  SST::Component::Params_t& params );
  Component* create_cpu( SST::ComponentId_t id,  SST::Component::Params_t& params );
}


static const ElementInfoComponent components[] = {
    { "VaultSimC",
      "Vault Component",
      NULL,
      VaultSimCAllocComponent
    },
    { "logicLayer",
      "Logic Layer Component",
      NULL,
      create_logicLayer
    },
    { "cpu",
      "simple CPU",
      NULL,
      create_cpu
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
