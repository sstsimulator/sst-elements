// Copyright 2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _STS_H
#define _STS_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <vector>

#include <queue>

#include <sst/core/interfaces/simpleMem.h>

namespace SST {
namespace GNAComponent {

//foward decl
class GNA;
class Request;

//  A Spike Transfer Structure engine - transforms a given spike by
//  performing a look up and delivering spikes
class STS {
    GNA *myGNA;
    int stsID;
    int numSpikes; // number of spikes yet to deliver
    std::queue<SST::Interfaces::SimpleMem::Request *> incomingReqs;
public:
    STS(GNA *parent, int n) : myGNA(parent), stsID(n), numSpikes(0) {;}
    bool isFree();
    void assign(int);
    void advance(uint);
    void returnRequest(SST::Interfaces::SimpleMem::Request *req) {
        incomingReqs.push(req);
    }
};

}
}

#endif // _STS_H
