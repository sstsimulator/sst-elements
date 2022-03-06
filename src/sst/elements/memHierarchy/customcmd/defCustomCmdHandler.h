// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_DEFCUSTOMCMDHANDLER_H_
#define _MEMHIERARCHY_DEFCUSTOMCMDHANDLER_H_

#include <string>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/stdMem.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/customcmd/customCmdMemory.h"

namespace SST {
namespace MemHierarchy {

/*
 * Default subcomponent for handling custom commands at memory controller
 * Simply copies custom cmd data structure from incoming event to
 * the memory controller backend
 *
 */
class DefCustomCmdMemHandler : public CustomCmdMemHandler {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(DefCustomCmdMemHandler, "memHierarchy", "defCustomCmdHandler", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Default, custom command handler that copies custom data to backend", SST::MemHierarchy::CustomCmdMemHandler)

/* Begin class defintion */

  DefCustomCmdMemHandler(ComponentId_t id, Params &params, std::function<void(Addr,size_t,std::vector<uint8_t>&)> read, std::function<void(Addr,std::vector<uint8_t>*)> write)
    : CustomCmdMemHandler(id, params, read, write) {}

  ~DefCustomCmdMemHandler() {}

  CustomCmdMemHandler::MemEventInfo receive(MemEventBase* ev) override;

  Interfaces::StandardMem::CustomData* ready(MemEventBase* ev) override;

  MemEventBase* finish(MemEventBase *ev, uint32_t flags) override;

protected:
private:
};    // class DefCustomCmdMemHandler
}     // namespace MemHierarchy
}     // namespace SST

#endif
