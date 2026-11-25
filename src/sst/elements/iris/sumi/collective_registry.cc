// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <iris/sumi/collective_registry.h>
#include <sst/core/factory.h>

namespace SST::Iris::sumi {

namespace {
std::string eliName(Collective::type_t ty, const std::string& name)
{
  auto dot = name.find('.');
  std::string lib = dot == std::string::npos ? "iris" : name.substr(0, dot);
  std::string alg = dot == std::string::npos ? name : name.substr(dot + 1);
  return lib + "." + Collective::tostr(ty) + "." + alg;
}
}

Collective*
CollectiveRegistry::make(Collective::type_t ty, const std::string& name,
                         const CollectiveFactoryArgs& args)
{
  if (name.empty()) return nullptr;
  auto type = eliName(ty, name);
  auto* factory = SST::Factory::getFactory();
  if (!factory->isSubComponentLoadableUsingAPI<Collective>(type)) return nullptr;
  return factory->Create<Collective>(type, args);
}

bool
CollectiveRegistry::has(Collective::type_t ty, const std::string& name)
{
  if (name.empty()) return false;
  return SST::Factory::getFactory()
      ->isSubComponentLoadableUsingAPI<Collective>(eliName(ty, name));
}

}
