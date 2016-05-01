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
#include <sst/core/interfaces/simpleNetwork.h>

#include <deque>


namespace SST {
namespace MemHierarchy {

using SST::Interfaces::SimpleNetwork;

class MemNetBridge : public SST::Component {
public:

    MemNetBridge(SST::ComponentId_t id, SST::Params &params);
    ~MemNetBridge();
    void init(unsigned int);
    void setup(void);
    void finish(void);


private:
    Output dbg;

    typedef std::map<std::string, SimpleNetwork::nid_t> addrMap_t;
    typedef std::map<std::string, uint64_t> imreMap_t;
    struct Nic_t {
        SimpleNetwork *nic;
        addrMap_t map;
        imreMap_t imreMap;
        std::deque<SimpleNetwork::Request*> sendQueue;


        Statistic<uint64_t> *stat_Recv;
        Statistic<uint64_t> *stat_Send;

        SimpleNetwork::nid_t getAddr() const { return nic->getEndpointID(); }
    };

    Nic_t interfaces[2];
    bool clockOn;
    TimeConverter* defaultTimeBase;
    Clock::Handler<MemNetBridge>* clockHandler;

    void configureNIC(uint8_t nic, SST::Params &params);
    bool handleIncoming(int vn, uint8_t nic);
    bool clock(SST::Cycle_t cycle);
    SimpleNetwork::nid_t getAddrFor(Nic_t &nic, const std::string &tgt);

};

}}

#endif
