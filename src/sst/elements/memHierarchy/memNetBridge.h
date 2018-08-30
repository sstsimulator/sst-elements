// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _MEMNETBRIDGE_H_
#define _MEMNETBRIDGE_H_


#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include <sst/elements/merlin/bridge.h>

#include <map>

namespace SST {
namespace MemHierarchy {

using SST::Interfaces::SimpleNetwork;

class MemNetBridge : public SST::Merlin::Bridge::Translator {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(MemNetBridge, "memHierarchy", "MemNetBridge", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Merlin::Bridge::Translator for memory network bridging", "SST::Merlin::Bridge::Translator")

    SST_ELI_DOCUMENT_PARAMS(
            {"debug",       "(int) Print debug information. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
            {"debug_level", "(int) Debugging level. Between 0 and 10", "0"} )

/* Begin class definition */
    MemNetBridge(SST::Component *comp, SST::Params &params);
    ~MemNetBridge();
    void init(unsigned int);
    void setup(void);
    void finish(void);

    SimpleNetwork::Request* translate(SimpleNetwork::Request* req, uint8_t fromNetwork);
    SimpleNetwork::Request* initTranslate(SimpleNetwork::Request* req, uint8_t fromNetwork);

private:
    Output dbg;

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
