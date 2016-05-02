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

#include <sst/elements/merlin/bridge.h>

#include <map>

namespace SST {
namespace MemHierarchy {

using SST::Interfaces::SimpleNetwork;

class MemNetBridge : public SST::Merlin::Bridge::Translator {
public:

    MemNetBridge(SST::Component *comp, SST::Params &params);
    ~MemNetBridge();
    void init(unsigned int);
    void setup(void);
    void finish(void);

    SimpleNetwork::Request* translate(SimpleNetwork::Request* req, uint8_t fromNetwork);
    SimpleNetwork::Request* initTranslate(SimpleNetwork::Request* req, uint8_t fromNetwork);

private:
    Output dbg;

    Merlin::Bridge *bridge;

    typedef std::map<std::string, SimpleNetwork::nid_t> addrMap_t;
    typedef std::map<std::string, uint64_t> imreMap_t;

    struct Net_t {
        addrMap_t map;
        imreMap_t imreMap;
    };

    Net_t networks[2];

    SimpleNetwork::nid_t getAddrFor(Net_t &nic, const std::string &tgt);

};

}}

#endif
