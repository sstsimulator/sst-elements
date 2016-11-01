// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef __SST_MEMH_MEMBACKENDCONVERTOR__
#define __SST_MEMH_MEMBACKENDCONVERTOR__

#include <sst/core/subcomponent.h>
#include "sst/elements/memHierarchy/memEvent.h"

namespace SST {
namespace MemHierarchy {

class MemBackend;

class MemBackendConvertor : public SubComponent {
  public:
    typedef uint64_t ReqId; 

  public:

    MemBackendConvertor();
    MemBackendConvertor(Component* comp, Params& params);
    void finish(void);
    virtual const std::string& getClockFreq();
    virtual size_t getMemSize();
    virtual bool clock( Cycle_t cycle ) = 0;
    virtual void handleMemEvent(  MemEvent* ) = 0;
    virtual const std::string& getRequestor( ReqId ) { assert(0); };

  protected:
    ~MemBackendConvertor() {}

    void doReceiveStat( Command cmd) {
        switch (cmd ) { 
            case GetS: 
                stat_GetSReqReceived->addData(1);
                break;
            case GetX:
                stat_GetXReqReceived->addData(1);
                break;
            case GetSEx:
                stat_GetSExReqReceived->addData(1);
                break;
            case PutM:
                stat_PutMReqReceived->addData(1);
                break;
            default:
                break;
        }
    }

    void doResponseStat( Command cmd, Cycle_t latency ) {
        switch (cmd) {
            case GetS:
                stat_GetSLatency->addData(latency);
                break;
            case GetSEx:
                stat_GetSExLatency->addData(latency);
                break;
            case GetX:
                stat_GetXLatency->addData(latency);
                break;
            case PutM:
                stat_PutMLatency->addData(latency);
                break;
            default:
                break;
        }
    }

    Output      m_dbg;
    MemBackend* m_backend;

    Statistic<uint64_t>* stat_GetSLatency;
    Statistic<uint64_t>* stat_GetSExLatency;
    Statistic<uint64_t>* stat_GetXLatency;
    Statistic<uint64_t>* stat_PutMLatency;

    Statistic<uint64_t>* stat_GetSReqReceived;
    Statistic<uint64_t>* stat_GetXReqReceived;
    Statistic<uint64_t>* stat_PutMReqReceived;
    Statistic<uint64_t>* stat_GetSExReqReceived;

    Statistic<uint64_t>* cyclesWithIssue;
    Statistic<uint64_t>* cyclesAttemptIssueButRejected;
    Statistic<uint64_t>* totalCycles;
    Statistic<uint64_t>* stat_outstandingReqs;

};

}
}
#endif
