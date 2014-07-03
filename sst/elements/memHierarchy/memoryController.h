// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   coherenceControllers.h
 * Author: Branden Moore Caesar De la Paz III
 * Email:  bjmoor@sandia.gov / caesar.sst@gmail.com
 */

#ifndef _MEMORYCONTROLLER_H
#define _MEMORYCONTROLLER_H


#include <sst/core/event.h>
#include <sst/core/module.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <map>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include "memEvent.h"

#include "bus.h"

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

class MemBackend;


class MemController : public SST::Component {
public:
    struct DRAMReq {
        void setIsWrite(bool write){ isWrite = write; }
        void setSize(size_t _size){ size = _size; }
        void setAddr(Addr _baseAddr){ baseAddr = _baseAddr; }
        enum Status_t {NEW, PROCESSING, RETURNED, DONE};

        MemEvent *reqEvent;
        MemEvent *respEvent;
        bool isWrite;
        bool canceled;
        bool isACK;
        bool responseNeeded;

        int respSize;
        Command cmd;
        
        size_t size;
        
        size_t amt_in_process;
        size_t amt_processed;
        Status_t status;

        Addr baseAddr;
        uint32_t num_req;

        DRAMReq(MemEvent *ev, const size_t busWidth, const size_t cacheLineSize) :
            reqEvent(new MemEvent(*ev)), respEvent(NULL),
            canceled(false), isACK(false), responseNeeded(false), amt_in_process(0), amt_processed(0), status(NEW){
            
            cmd = ev->getCmd();
            if(cmd == PutM) isWrite = true;
            else isWrite = false;
            
            if(cmd == GetS || cmd == GetSEx || cmd == GetX) responseNeeded = true;

            setSize(cacheLineSize);
            baseAddr = ev->getBaseAddr();
            
#if 0
            printf(
                    "***************************************************\n"
                    "Buswidth = %zu\n"
                    "Ev:   0x%08llx  + 0x%02x -> 0x%08llx\n"
                    "Req:  0x%08llx  + 0x%02x   [%u count]\n"
                    "***************************************************\n",
                    busWidth, ev->getAddr(), ev->getSize(), reqEndAddr,
                    addr, size);
#endif
        }

        ~DRAMReq() {
            delete reqEvent;
        }

        bool isSatisfiedBy(const MemEvent *ev)
        {
            if ( isACK ) return false;
            return ((reqEvent->getAddr() >= ev->getAddr()) &&
                    (reqEvent->getAddr()+reqEvent->getSize() <= (ev->getAddr() + ev->getSize())));
        }

    };

    MemController(ComponentId_t id, Params &params);
    void init(unsigned int);
    void setup();
    void finish();

    void handleMemResponse(DRAMReq *req);

    Output dbg;

private:



    MemController();  // for serialization only
    ~MemController();

    void handleEvent(SST::Event *event);
    void handleBusEvent(SST::Event *event);

    void addRequest(MemEvent *ev);
    void cancelEvent(MemEvent *ev);
    bool clock(SST::Cycle_t cycle);

    bool isRequestAddressValid(MemEvent *ev);
    Addr convertAddressToLocalAddress(Addr addr);
    void performRequest(DRAMReq *req);

    void sendBusPacket(Bus::key_t key);
    void sendBusCancel(Bus::key_t key);

    void sendResponse(DRAMReq *req);

    void handleCubeEvent(SST::Event *event);
    
    void printMemory(DRAMReq *req, Addr localAddr);

    bool divert_DC_lookups;
    bool use_dramsim;
    bool use_vaultSim;
    bool use_hybridsim;

    SST::Link *upstream_link;
    MemBackend *backend;
    bool use_bus;
    bool bus_requested;
    std::deque<DRAMReq*> busReqs;
    int protocol;

    typedef std::deque<DRAMReq*> dramReq_t;
    dramReq_t requestQueue;
    dramReq_t requests;

    int backing_fd;
    uint8_t *memBuffer;
    size_t memSize;
    size_t requestSize;
    Addr rangeStart;
    size_t numPages;
    Addr interleaveSize;
    Addr interleaveStep;
    bool respondToInvalidates;
    size_t cacheLineSize;
    size_t requestWidth;

#ifdef HAVE_LIBZ
    gzFile traceFP;
#else
    FILE *traceFP;
#endif

    Output::output_location_t statsOutputTarget;
    uint64_t GetSReqReceived;
    uint64_t GetXReqReceived;
    uint64_t PutMReqReceived;
    uint64_t GetSExReqReceived;
    uint64_t numReqOutstanding;
    uint64_t numCycles;

};

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

#endif /* _MEMORYCONTROLLER_H */
