// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MEMNIC_H_
#define _MEMHIERARCHY_MEMNIC_H_

#include <string>
#include <deque>


#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/interfaces/memEvent.h>

#include <sst/elements/merlin/linkControl.h>

using namespace SST::Interfaces;

namespace SST {
namespace MemHierarchy {

class MemNIC {

public:
    enum ComponentType {
        TypeCache,
        TypeDirectoryCtrl,
        TypeOther
    };

    union ComponentTypeInfo {
        struct {
            uint32_t blocksize;
            uint32_t num_blocks;
        } cache;
        struct {
            uint64_t rangeStart;
            uint64_t rangeEnd;
            uint64_t interleaveSize;
            uint64_t interleaveStep;
        } dirctrl;
    };

    struct ComponentInfo {
        std::string link_port;
        std::string link_bandwidth;
        std::string name;
        int network_addr;
        ComponentType type;
        ComponentTypeInfo typeInfo;
    };


private:
    class MemRtrEvent : public Merlin::RtrEvent {
    public:
        MemEvent event;

        MemRtrEvent(MemEvent *ev) :
            Merlin::RtrEvent(), event(*ev)
        { }

        virtual RtrEvent* clone(void) {
            return new MemRtrEvent(*this);
        }
    };

    class InitMemRtrEvent : public Merlin::RtrEvent {
    public:
        std::string name;
        int address;
        ComponentType compType;
        ComponentTypeInfo compInfo;

        InitMemRtrEvent(const std::string &name, int addr, ComponentType type, ComponentTypeInfo info) :
            Merlin::RtrEvent(), name(name), address(addr), compType(type), compInfo(info)
        {
            src = addr;
        }

        virtual RtrEvent* clone(void) {
            return new InitMemRtrEvent(*this);
        }
    };


    static const int num_vcs;
    size_t flitSize;

    Component *comp;
    ComponentInfo ci;
    Event::HandlerBase *recvHandler;
    Merlin::LinkControl *link_control;

    std::deque<Event*> initQueue;
    std::deque<MemRtrEvent *> sendQueue;
    int last_recv_vc;
    std::map<std::string, int> addrMap;
    /* Built during init -> available in Setup and later */
    std::vector<ComponentInfo> peers;


    /* Translates a MemEvent string destination to an integer */
    int addrForDest(const std::string &target);

    /* Get size in flits for a MemEvent */
    int getFlitSize(MemEvent *ev);


public:
    MemNIC(Component *comp, ComponentInfo &ci, Event::HandlerBase *handler);


    /* Call these from their respective calls in the component */
    void setup(void);
    void init(unsigned int phase);
    void clock(void);


    void send(MemEvent *ev);
    void sendInitData(MemEvent *ev);
    SST::Event* recvInitData(void);
    const std::vector<ComponentInfo>& getPeerInfo(void) const { return peers; }
    void clearPeerInfo(void) { peers.clear(); }

};

}
}

#endif
