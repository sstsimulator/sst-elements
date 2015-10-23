#ifndef VAULTSIM_H
#define VAULTSIM_H

#include "macsim.h"

namespace HMC {
  class LogicLayer;
  LogicLayer *get_memory_system_instance(unsigned id, bool terminal, macsim_c *simBase);
} // namespace HMC

#endif
