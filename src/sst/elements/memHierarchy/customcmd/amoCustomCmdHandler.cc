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

#include <sst_config.h>

#include "customcmd/amoCustomCmdHandler.h"
#include "customcmd/customOpCodeCmd.h"
#include "customcmd/customCmdEvent.h"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

CustomCmdMemHandler::MemEventInfo AMOCustomCmdMemHandler::receive(MemEventBase* ev){
    CustomCmdMemHandler::MemEventInfo MEI(ev->getRoutingAddress(),true);
    return MEI;
}

CustomCmdInfo* AMOCustomCmdMemHandler::ready(MemEventBase* ev){
    CustomCmdEvent * cme = static_cast<CustomCmdEvent*>(ev);
    CustomOpCodeCmdInfo *CI = new CustomOpCodeCmdInfo(cme->getID(),
                                        cme->getRqstr(),
                                        cme->getAddr(),
                                        cme->getOpCode(),
                                        MemEventBase::F_SUCCESS);
    return CI;
}

MemEventBase* AMOCustomCmdMemHandler::finish(MemEventBase *ev, uint32_t flags){
    if(ev->queryFlag(MemEventBase::F_NORESPONSE)||
         ((flags & MemEventBase::F_NORESPONSE)>0)){
        // posted request
        return nullptr;
    }

    MemEventBase *MEB = ev->makeResponse();
    return MEB;
}

// EOF
