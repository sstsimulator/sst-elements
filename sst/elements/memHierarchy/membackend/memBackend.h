//
//  memBackend.h
//  SST
//
//  Created by Caesar De la Paz III on 7/9/14.
//  Copyright (c) 2014 De la Paz, Cesar. All rights reserved.
//

#ifndef __SST__memBackend__
#define __SST__memBackend__

#include <sst/core/event.h>

#include <iostream>
#include <map>

#include "memoryController.h"

#if defined(HAVE_LIBDRAMSIM)
// DRAMSim uses DEBUG
#ifdef DEBUG
# define OLD_DEBUG DEBUG
# undef DEBUG
#endif
#include <DRAMSim.h>
#ifdef OLD_DEBUG
# define DEBUG OLD_DEBUG
# undef OLD_DEBUG
#endif
#endif

// MARYLAND CHANGES
#if defined(HAVE_LIBHYBRIDSIM)
// HybridSim also uses DEBUG
#ifdef DEBUG
# define OLD_DEBUG DEBUG
# undef DEBUG
#endif
#include <HybridSim.h>
#ifdef OLD_DEBUG
# define DEBUG OLD_DEBUG
# undef OLD_DEBUG
#endif
#endif

namespace SST {
namespace MemHierarchy {

class MemController;

class MemBackend : public Module {
public:
    MemBackend();
    MemBackend(Component *comp, Params &params);
    virtual bool issueRequest(MemController::DRAMReq *req) = 0;
    virtual void setup() {}
    virtual void finish() {}
    virtual void clock() {}
protected:
    MemController *ctrl;
};



class SimpleMemory : public MemBackend {
public:
    SimpleMemory();
    SimpleMemory(Component *comp, Params &params);
    bool issueRequest(MemController::DRAMReq *req);
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

    void handleSelfEvent(SST::Event *event);

    Link *self_link;
};


#if defined(HAVE_LIBDRAMSIM)
class DRAMSimMemory : public MemBackend {
public:
    DRAMSimMemory(Component *comp, Params &params);
    bool issueRequest(MemController::DRAMReq *req);
    void clock();
    void finish();

private:
    void dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

    DRAMSim::MultiChannelMemorySystem *memSystem;
    std::map<uint64_t, std::deque<MemController::DRAMReq*> > dramReqs;
};
#endif


#if defined(HAVE_LIBHYBRIDSIM)
class HybridSimMemory : public MemBackend {
public:
    HybridSimMemory(Component *comp, Params &params);
    bool issueRequest(MemController::DRAMReq *req);
    void clock();
    void finish();
private:
    void hybridSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

    HybridSim::HybridSystem *memSystem;
    std::map<uint64_t, std::deque<MemController::DRAMReq*> > dramReqs;
};
#endif




class VaultSimMemory : public MemBackend {
public:
    VaultSimMemory(Component *comp, Params &params);
    bool issueRequest(MemController::DRAMReq *req);
private:
    void handleCubeEvent(SST::Event *event);

    typedef std::map<MemEvent::id_type,MemController::DRAMReq*> memEventToDRAMMap_t;
    memEventToDRAMMap_t outToCubes; // map of events sent out to the cubes
    SST::Link *cube_link;
};

}}

#endif /* defined(__SST__memBackend__) */

