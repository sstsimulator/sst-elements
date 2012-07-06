// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_HR_ROUTER_HR_ROUTER_H
#define COMPONENTS_HR_ROUTER_HR_ROUTER_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <queue>

#include "sst/elements/merlin/router.h"

using namespace SST;

typedef std::queue<internal_router_event*> port_queue_t;

class port_buffer {
private:
    port_queue_t* buffers;
    int* tokens;
    int num_vcs;
    
public:
    port_buffer(int num_vcs);
    ~port_buffer();

    void setTokens(int vc, int num_tokens);
    
};

class hr_router : public Component {

private:
    int id;
    int rows;
    int columns;
    int num_ports;
    int num_vcs;

    Topology* topo;
    
    port_buffer** input_bufs;
    port_buffer** output_bufs;

    Link** links;

    void port_handler(Event* event, int port);
    
public:
    hr_router(ComponentId_t cid, Params& params);
    ~hr_router();
    
    int Setup() {return false;}
    int Finish() {return false;}
};

#endif // COMPONENTS_HR_ROUTER_HR_ROUTER_H
