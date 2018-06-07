// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_AMOCUSTOMCMDHANDLER_H_
#define _MEMHIERARCHY_AMOCUSTOMCMDHANDLER_H_

#include <string>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/elementinfo.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/customcmd/customCmdMemory.h"

namespace SST {
namespace MemHierarchy {

/*
 * Atomic Memory Operation (AMO)
 * Custom Command Handler
 */
class AMOCustomCmdMemHandler : public CustomCmdMemHandler {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(AMOCustomCmdMemHandler, "memHierarchy", "amoCustomCmdHandler", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Custom command handler for atomics (AMO)", "SST::MemHierarchy::CustomCmdMemHandler")

/* Begin class defintion */
  AMOCustomCmdMemHandler(Component * comp, Params &params)
    : CustomCmdMemHandler(comp,params) {}

  ~AMOCustomCmdMemHandler() {}

  CustomCmdMemHandler::MemEventInfo receive(MemEventBase* ev) override;

  CustomCmdInfo* ready(MemEventBase* ev) override;

  MemEventBase* finish(MemEventBase *ev, uint32_t flags) override;

protected:
private:
};    // class AMOCustomCmdMemHandler
}     // namespace MemHierarchy
}     // namespace SST

#endif
