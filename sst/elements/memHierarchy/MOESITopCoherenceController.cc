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

/*
 * File:   MESITopCoherenceController.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include <vector>
#include "coherenceControllers.h"
#include "MOESITopCoherenceController.h"

using namespace SST;
using namespace SST::MemHierarchy;


/*-------------------------------------------------------------------------------------
 * MOESI Top Coherence Controller Implementation (for L2 and below)
 *-------------------------------------------------------------------------------------*/

/*
 *  Return true if request has been handled
 */
bool MOESITopCC::handleRequest(MemEvent* _event, CacheLine* _cacheLine, bool _mshrHit) {
    Command cmd = _event->getCmd();
    bool ret = false;

    switch(cmd) {
        case GetS:
            break;
        case GetX:
        case GetSEx:
            break;
        case PutS:
            break;
        case PutM:
        case PutX:
            break;
        case PutE:
        case PutXE:
            break;
        default:
            d_->fatal(CALL_INFO, -1, "Unrecognized command");
    }
    return ret;
}

bool MOESITopCC::handleGetSRequest(MemEvent* _event, CacheLine* _cacheLine, bool _mshrHit) {
//    State state             = _cacheLine->getState();
//    vector<uint8_t>* data   = _cacheLine->getData();
//    int lineIndex           = _cacheLine->getIndex();
    //CCLine* l               = ccLines_[lineIndex];      // owner/sharer data

    /* We own the block and no other cache has it -> send block in E state */
   /* if (!l->ownerExists() && l->isShareless() && (state == E || state == M || state == O)) {
        //l->setOwner(_sharerID);
        //return sendResponse(_event, E, data, _mshrHit);
    } else if (l->ownerExists) {
        if (state == M || state == E) { // Invalidate or downgrade and get data
            //sendInvalidateX(lineIndex, _event->getRqstr(), _mshrHit);
        } else if (state == O) {
        } else {
            d_->fatal(CALL_INFO, -1, "Block has owner but is not in an owned state");
        }
    } else if (state == S || state == O) {
        //l->addSharer(_sharerId);
        //return sendResponse(_event, S, data, _mshrHit);
    }*/
    return false;
}
