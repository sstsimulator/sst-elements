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

#include <sst_config.h>

#include "customcmd/defCustomCmdHandler.h"
#include "memEventCustom.h"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

CustomCmdMemHandler::MemEventInfo DefCustomCmdMemHandler::receive(MemEventBase* ev){
    CustomCmdMemHandler::MemEventInfo MEI(ev->getRoutingAddress(),true);
    return MEI;
}

Interfaces::StandardMem::CustomData* DefCustomCmdMemHandler::ready(MemEventBase* ev){
    CustomMemEvent * cme = static_cast<CustomMemEvent*>(ev);
    // We don't need to modify the data structure sent by the CPU, so just 
    // pass it on to the backend
    return cme->getCustomData();
}

MemEventBase* DefCustomCmdMemHandler::finish(MemEventBase *ev, uint32_t flags){
    if(ev->queryFlag(MemEventBase::F_NORESPONSE)||
         ((flags & MemEventBase::F_NORESPONSE)>0)){
        // posted request
        // We need to delete the CustomData structure
        CustomMemEvent * cme = static_cast<CustomMemEvent*>(ev);
        if (cme->getCustomData() != nullptr)
            delete cme->getCustomData();
        cme->setCustomData(nullptr); // Just in case someone attempts to access it...
        return nullptr;
    }

    MemEventBase *MEB = ev->makeResponse();
    return MEB;
}

// EOF
