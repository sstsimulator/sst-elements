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

#ifndef _SIMPLETRACERCOMPONENT_H
#define _SIMPLETRACERCOMPONENT_H

#include <sst/elements/memHierarchy/memEvent.h>

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

namespace SST{
namespace SimpleTracerComponent {

class simpleTracerComponent : public SST::Component {
public:
    simpleTracerComponent(SST::ComponentId_t id, Params& params);
    ~simpleTracerComponent();
    void finish();

private:
    // Functions
    bool clock(SST::Cycle_t);
    void FinalStats(FILE*, unsigned int);
    void PrintAddrHistogram(FILE*, vector<SST::MemHierarchy::Addr>);
    void PrintAccessLatencyDistribution(FILE*, unsigned int);

    Output* out;
    FILE* traceFile;
    FILE* statsFile;

    // Links
    SST::Link *northBus;
    SST::Link *southBus;

    // Input Parameters
    unsigned int stats;
    unsigned int pageSize;
    unsigned int accessLatBins;

    // Flags
    bool writeTrace;
    bool writeStats;
    bool writeDebug_8;

    unsigned int nbCount;
    unsigned int sbCount;
    uint64_t timestamp;

    vector<SST::MemHierarchy::Addr>AddrHist;   // Address Histogram
    vector<unsigned int> AccessLatencyDist;

    map<MemEvent::id_type,uint64_t>InFlightReqQueue;

    TimeConverter* picoTimeConv;
    TimeConverter* nanoTimeConv;

    // Serialization
    simpleTracerComponent();                         // for serialization only
    simpleTracerComponent(const simpleTracerComponent&);      // do not implement
    void operator=(const simpleTracerComponent&);    // do not implement
}; // class simpleTracerComponent

} // namespace SimpleTracerComponent
} // namespace SST

#endif /* _SIMPLETRACERCOMPONENT_H */

