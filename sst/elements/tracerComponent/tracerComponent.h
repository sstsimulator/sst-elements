#ifndef _TRACERCOMPONENT_H
#define _TRACERCOMPONENT_H

#include <sst/core/output.h>
#include <sst/core/debug.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/sst_types.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <assert.h>
#include <errno.h>
#include <execinfo.h>

#include <iostream>
#include <fstream>
#include <map>

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

namespace SST{
namespace TRACERCOMPONENT {

class tracerComponent : public SST::Component {
public:
    tracerComponent(SST::ComponentId_t id, Params& params);
    ~tracerComponent();
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
    tracerComponent();                         // for serialization only
    tracerComponent(const tracerComponent&);      // do not implement
    void operator=(const tracerComponent&);    // do not implement
}; // class tracerComponent

} // namespace TRACERCOMPONENT
} // namespace SST

#endif //_TRACERCOMPONENT_H

