// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_PAGEDMULTI_BACKEND
#define _H_SST_MEMH_PAGEDMULTI_BACKEND

#include "membackend/dramSimBackend.h"

#ifdef DEBUG
#define OLD_DEBUG DEBUG
#undef DEBUG
#endif

#include <DRAMSim.h>

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

struct pageInfo {
    uint touched; // how many times it is touched in quanta
    bool inFast;
    pageInfo() : touched(0), inFast(0) {;}
};

class pagedMultiMemory : public DRAMSimMemory {
public:
    pagedMultiMemory(Component *comp, Params &params);
    virtual bool issueRequest(MemController::DRAMReq *req);
    virtual void clock();
    virtual void finish();

private:
class MemCtrlEvent : public SST::Event {
    public:
        MemCtrlEvent(MemController::DRAMReq* req) : SST::Event(), req(req)
        { }

        MemController::DRAMReq *req;
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
            ar & BOOST_SERIALIZATION_NVP(req);
        }
    };

    typedef map<uint64_t, pageInfo> pageMap_t;
    pageMap_t pageMap;
    int maxFastPages;
    int pagesInFast;

    void handleSelfEvent(SST::Event *event);
    bool quantaClock(SST::Cycle_t _cycle);
    Link *self_link;

    // statistics
    Statistic<uint64_t> *fastHits;
    Statistic<uint64_t> *fastSwaps;
    Statistic<uint64_t> *fastAccesses;
};

}
}

#endif
