// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   broadcastShim.h
 */

#ifndef _MEMH_SIEVE_BROADCASTSHIM_H_
#define _MEMH_SIEVE_BROADCASTSHIM_H_

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>


#include "../util.h"
#include "../../ariel/arielalloctrackev.h"


namespace SST { namespace MemHierarchy {

using namespace std;


class BroadcastShim : public SST::Component {
public:

    /** Constructor */
    BroadcastShim(ComponentId_t id, Params &params);
    
    /** Destructor for Sieve Component */
    ~BroadcastShim() {}
    
    virtual void init(unsigned int) {}
    virtual void finish(void) {}
    
    
private:

    /** Handler for incoming allocation events.  */
    void processCoreEvent(SST::Event* event);
    void processSieveEvent(SST::Event* event);

    SST::Output* output_;
    vector<SST::Link*>  cpuAllocLinks_;
    vector<SST::Link*>  sieveAllocLinks_;
};

}}

#endif
