// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _MEMNETBRIDGE_H_
#define _MEMNETBRIDGE_H_


#include <sst/core/Component.h>
#include <sst/core/Event.h>
#include <sst/core/Output.h>

#include "memNIC.h"

namespace SST {
namespace MemHierarchy {

class MemNetBridge : public SST::Component {
public:

    MemNetBridge(SST::ComponentId_t id, SST::Params &params);
    ~MemNetBridge();
    void init(unsigned int);
    void setup(void);
    void finish(void);


private:
    Output dbg;

    MemNIC* interfaces[2];
    size_t  peerCount[2];  /* Used during init */


    void configureNIC(uint8_t nic, SST::Params &params);
    void updatePeerInfo(uint8_t nic);
    void handleIncoming(SST::Event *event, uint8_t nic);

};

}}

#endif
